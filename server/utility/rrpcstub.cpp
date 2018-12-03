#include "rrpcstub.h"

void rrpc_dispatch_loop_run(void* arg) {
	rrpcstub* stub = static_cast<rrpcstub*>(arg);
	stub->dispatch();
}

rrpcstub::rrpcstub():_session(0),unsize(0) {
	uv_thread_t tid;
	uv_thread_create(&tid, rrpc_dispatch_loop_run, this);
}

rrpcstub::~rrpcstub() {
	while(_compque.size() > 0) {
		//free(_compque.front());
		delete[] _compque.front();
		_compque.pop_front();
	}
}

void rrpcstub::dispatch() {
	while(true) {
		while (_compque.size() == 0) _comcond.wait(3*CONDSEC/10);

		_comtx.lock();
		char* msg = _compque.front();
		_compque.pop_front();
		_comtx.unlock();

		if (onmessage(msg)) {
			//printf("collect msg\n");
			//free(msg);
			delete[] msg;
		}
	}
}

void rrpcstub::pushmessage(char* buff, int len) {
	// TODO: multiple packages
	if (unsize > 0) {
		memcpy(uncomp+unsize, buff, len);
		unsize += len;
		buff = uncomp;
		len = unsize;
	}
	while (len > 3) {
		RRpcMessageWrapper* wrap = (RRpcMessageWrapper*)buff;
		unsigned short size = wrap->Size();
		if (len >= (size+4)) {
			//char* comp = (char*)malloc(size+4);
			char* comp = new char[size+4];
			memcpy(comp, buff, size+4);
			_comtx.lock();
			_compque.push_back(comp);
			_comtx.unlock();
			len -= (size+4);
			buff = buff+size+4;
		} else {
			break;
		}
	}
	if (unsize > 0) {
		if (len == 0) {
			unsize = 0;
		} else if (unsize > len) {
			memmove(uncomp, uncomp+unsize-len, len);
		}
	} else if (len > 0) {
		memcpy(uncomp, buff, len);
		unsize = len;
	}
	if (_compque.size() > 0) _comcond.post();
}

char* rrpcstub::call(uvpp::client& c, RRpcMessage& msg, unsigned short timeout) {
	uvpp::condition cond;
	_syncmtx.lock();
	_syncond.insert(std::pair<short, uvpp::condition*>(msg.sid, &cond));
	_syncmtx.unlock();

	msg.iscall = true;
	int status = c.send(msg.data, msg.size, (void*)&msg);
	if (status != 0 || cond.wait(timeout*CONDSEC) != 0) {
		_syncmtx.lock();
		_syncond.erase(msg.sid);
		_syncmtx.unlock();
		return 0;
	}

	std::map<short,char*>::iterator it = _syncmsg.find(msg.sid);
	if (it == _syncmsg.end()) {
		_syncmtx.lock();
		_syncond.erase(msg.sid);
		_syncmtx.unlock();
		return 0;
	}

	char* ret = it->second;
	_syncmtx.lock();
	_syncmsg.erase(msg.sid);
	_syncond.erase(msg.sid);
	_syncmtx.unlock();
	return ret;
}

bool rrpcstub::resumecall(short sid) {
	_syncmtx.lock();
	std::map<short, uvpp::condition*>::iterator it = _syncond.find(sid);
	if (it != _syncond.end()) {
		it->second->post();
		_syncmtx.unlock();
		return true;
	}
	_syncmtx.unlock();
	return false;
}

bool rrpcstub::syncmessage(short sid, char* msg) {
	_syncmtx.lock();
	_syncmsg.insert(std::pair<short,char*>(sid, msg));
	_syncmtx.unlock();
	if (!resumecall(sid)) {
		_syncmtx.lock();
		_syncmsg.erase(sid);
		_syncmtx.unlock();
		return true; // to free msg in distpatch
	}
	return false;
}
