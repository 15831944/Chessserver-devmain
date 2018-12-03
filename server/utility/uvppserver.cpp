#include "uvppserver.h"

static void srv_connection_cb(uv_stream_t* server, int status) {
	uvpp::server* srv = static_cast<uvpp::server*>(server->data);
	srv->onconnect(status);
}

namespace uvpp {
	server::server(itcpserver* handle) {
		_tcp.data = this;
		_accept.data = this;
		_server = handle;
	}

	bool server::start(const char* host, unsigned short port) {
		struct sockaddr_in addr4;
		struct sockaddr_in6 addr6;

		struct sockaddr* addr = (struct sockaddr*)&addr6;
		int ret = uv_ip6_addr(host, port, &addr6);
		if (ret != 0) {
			ret = uv_ip4_addr(host, port, &addr4);
			if (ret != 0) {
				printf("host address %s:%d error %d\n", host, port, ret);
				return false;
			}
			addr = (struct sockaddr*)&addr4;
		}

		ret = uv_tcp_init(uvpp_loop, &_tcp);
		if (ret) {
			printf("Socket creation error %d\n", ret);
			return false;
		}

		ret = uv_tcp_bind(&_tcp, addr, 0);
		if (ret) {
			printf("Socket bind error %d\n", ret);
			return false;
		}

		ret = uv_listen((uv_stream_t*)&_tcp, SOMAXCONN, srv_connection_cb);
		if (ret) {
			printf("Socket listen error %d\n", ret);
			return false;
		}
		return true;
	}

	void server::onconnect(int status) {
		if (status != 0) {
			printf("Connect error %s\n", uv_err_name(status));
			return;
		}
		srvconn* c = _conns.get();
		c->reset();
		int ret = uv_tcp_init(uvpp_loop, (uv_tcp_t*)&c->_socket);;
		if (ret != 0) {
			printf("uv_tcp_init client error %d\n", ret);
			_conns.put(c);
			return ;
		}

		ret = uv_accept((uv_stream_t*)&_tcp, (uv_stream_t*)&c->_socket);
		if (ret != 0) {
			printf("uv_accept client error %d\n", ret);
			_conns.put(c);
			return ;
		}

		if (_server) {
			_server->onaccept(c);
		}

		ret = uv_read_start((uv_stream_t*)&c->_socket, uvpp_alloc_cb, uvpp_read_cb);
		if (ret != 0) {
			printf("uv_read_start client error %d\n", ret);
			c->close();
			if (_server) {
				_server->onclosed(c);
			}
			_conns.put(c);
			return ;
		}
		c->_server = this;
	}
}