#ifndef UVPP_HEAD
#define UVPP_HEAD

#include "uv.h"

extern uv_loop_t* uvpp_loop;

bool uvpp_init();
void uvpp_run_loop();
void uvpp_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
void uvpp_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
void uvpp_write_cb(uv_write_t* req, int status);
void uvpp_timer_cb(uv_timer_t* handle);
void uvpp_shutdown_cb(uv_shutdown_t* req, int status);

#include "uvppsync.h"
#include "uvppobjpool.h"

namespace uvpp {
	struct itcpclient {
		virtual void onconnect(int status) = 0;
		virtual void onread(char* buff, ssize_t len) = 0;
		virtual void afterwrite(void* arg, int status) = 0;
		virtual void aftershut(int status) {}
	};

	struct itimer {
		virtual void ontimer() = 0;
	};

	struct connection {
		connection();
		~connection();

		virtual int send(char* buff, ssize_t len, void* arg=0);

		virtual char* readbuff(ssize_t suggest, ssize_t* size);
		virtual void onread(ssize_t nread, char* buff, ssize_t len, uv_stream_t* stream);
		virtual void afterwrite(uv_write_t* req, int status);
		virtual void aftershut(uv_shutdown_t* req, int status);

		uv_tcp_t _socket;
		uv_shutdown_t _shutdown;
		uvpp::itcpclient* _handle;

		char _rbuff[65535];
		uvpp::semaphore _rsem;
		uvpp::objpool<uv_write_t, false> _writers;
	};
};

#include "uvpptimer.h"
#include "uvppclient.h"
#include "uvppserver.h"

#endif
