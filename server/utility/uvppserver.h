#ifndef UVPP_SERVER_HEAD
#define UVPP_SERVER_HEAD

#include "uvpp.h"

namespace uvpp {
	struct srvconn : public uvpp::connection {
		srvconn() {
			_server = 0;
		}

		void reset() {
			_server = 0;
			_handle = 0;
			memset(&_socket, 0, sizeof(_socket));
			memset(&_shutdown, 0, sizeof(_shutdown));
			_socket.data = this;
		}

		void close() {
			uv_read_stop((uv_stream_t*)&_socket);
			uv_close((uv_handle_t*)&_socket, 0);
		}

		void* _server;
	};

	struct itcpserver {
		virtual void onaccept(uvpp::srvconn* c);
		virtual void onclosed(uvpp::srvconn* c);
	};

	class server {
	public:
		server(itcpserver* handle);

		bool start(const char* addr, unsigned short port);
		void onconnect(int status);
		void takeback(uvpp::srvconn* c) {
			if (c) _conns.put(c);
		}

	private:
		uv_tcp_t _tcp;
		uv_stream_t _accept;

		uvpp::objpool<uvpp::srvconn> _conns;
		uvpp::itcpserver* _server;
	};
};

#endif
