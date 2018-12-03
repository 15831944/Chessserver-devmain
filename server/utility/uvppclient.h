#ifndef UVPP_CLIENT_HEAD
#define UVPP_CLIENT_HEAD

#include "uvpp.h"

namespace uvpp {
	class client: public uvpp::connection {
	public:
		client(uvpp::itcpclient* c);

		void connect(const char* host, unsigned short port);
		void onconnect(int status);
		bool iswritable() {
			return uv_is_writable((const uv_stream_t*)&_socket) != 0;
		}
		bool isreadable() {
			return uv_is_readable((const uv_stream_t*)&_socket) != 0;
		}

	private:
		uv_connect_t _connect;
	};
};

#endif
