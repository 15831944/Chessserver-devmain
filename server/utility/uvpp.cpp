#include "uvpp.h"

uv_loop_t* uvpp_loop = 0;
static uv_thread_t tid_loop = 0;
static uv_mutex_t uvpp_mtx_loop;

void uvpp_default_loop_run(void* arg) {
	uv_run(uvpp_loop, UV_RUN_DEFAULT);
	tid_loop = 0;
	printf("uv loop end!!!!!!!!!!!!\n");
}

bool uvpp_init() {
	if (!uvpp_loop) {
		uvpp_loop = uv_default_loop();
		uv_mutex_init(&uvpp_mtx_loop);
		return true;
	}
	return false;
}

void uvpp_run_loop() {
	uv_mutex_lock(&uvpp_mtx_loop);
	if (!tid_loop) {
		uv_thread_create(&tid_loop, uvpp_default_loop_run, 0);
	}
	uv_mutex_unlock(&uvpp_mtx_loop);
}

void uvpp_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	uvpp::connection* conn = static_cast<uvpp::connection*>(stream->data);
	conn->onread(nread, buf ? buf->base:0, buf ? buf->len:0, stream);
}

void uvpp_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	uvpp::connection* alloc = static_cast<uvpp::connection*>(handle->data);
	buf->base = alloc->readbuff(suggested_size, (ssize_t*)&(buf->len));
}

void uvpp_write_cb(uv_write_t* req, int status) {
	uvpp::connection* conn = static_cast<uvpp::connection*>(req->handle->data);
	conn->afterwrite(req, status);
}

void uvpp_shutdown_cb(uv_shutdown_t* req, int status) {
	uvpp::connection* conn = static_cast<uvpp::connection*>(req->handle->data);
	conn->aftershut(req, status);
}

void uvpp_timer_cb(uv_timer_t* handle) {
	uvpp::itimer* timer = static_cast<uvpp::itimer*>(handle->data);
	timer->ontimer();
}

namespace uvpp {
	connection::connection() {
		_handle = 0;
		memset(&_socket, 0, sizeof(_socket));
		memset(&_shutdown, 0, sizeof(_shutdown));
		_socket.data = this;
	}

	connection::~connection() {
		uv_close((uv_handle_t*)&_socket, 0);
	}

	// buff's life should keeped until uvpp_write_cb fired
	int connection::send(char* buff, ssize_t len, void* arg) {
		uv_write_t* req = _writers.get();
		req->data = arg;
		uv_buf_t buf_t = {(ULONG)len, buff};
		int status = uv_write(req, (uv_stream_t*)&_socket, &buf_t, 1, uvpp_write_cb);
		if (status != 0) {
			afterwrite(req, status);
		}
		return status;
	}

	char* connection::readbuff(ssize_t suggest, ssize_t* size) {
		_rsem.wait();
		*size = suggest;
		if (suggest > sizeof(_rbuff)) {
		  *size = sizeof(_rbuff);
		}
		return _rbuff;
	}

	void connection::onread(ssize_t nread, char* buff, ssize_t len, uv_stream_t* stream) {
		printf("connection::onread %d\n", nread);
		if (nread < 1 || !buff || len == 0) {
		  uv_read_stop((uv_stream_t*)&_socket);
		  uv_shutdown(&_shutdown, (uv_stream_t*)&_socket, uvpp_shutdown_cb);

		  if (_handle) _handle->onread(0, 0);
		  _rsem.post();
		  return ;
		}
		if (_handle) _handle->onread(buff, nread);
		_rsem.post();
	}

	void connection::afterwrite(uv_write_t* req, int status) {
		if (_handle) _handle->afterwrite(req->data, status);
		_writers.put(req);
		if (status != 0) {
		  printf("uvpp connection wrte error %d:%s\n", status, uv_strerror(status));
		}
	}

	void connection::aftershut(uv_shutdown_t* req, int status) {
		printf("shutdown ret %d\n", status);
		uv_close((uv_handle_t*)&_socket, 0);
		if (_handle) _handle->aftershut(status);
		// writable event will fired after this callback
	}
};
