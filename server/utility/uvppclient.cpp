#include "uvppclient.h"

void c_on_connect(uv_connect_t* req, int status) {
	uvpp::client* cli = static_cast<uvpp::client*>(req->data);
	cli->onconnect(status);
}

namespace uvpp {
	client::client(uvpp::itcpclient* c) {
		_handle = c;
		_connect.data = this;
	}

	void client::connect(const char* host, unsigned short port) {
		struct sockaddr_in dest;
		uv_ip4_addr(host, port, &dest);

		if (uv_tcp_init(uvpp_loop, &_socket) == 0 &&
		 uv_tcp_connect(&_connect, &_socket, (const struct sockaddr*)&dest, c_on_connect) == 0) {
		 	uvpp_run_loop();
		}
	}

	void client::onconnect(int status) {
		if (status == 0) {
			uv_read_start((uv_stream_t*)&_socket, uvpp_alloc_cb, uvpp_read_cb);
		} else {
			uv_close((uv_handle_t*)&_socket, 0);
		}
		if (_handle) _handle->onconnect(status);
	}
}