#ifndef RAW_RPC_STUB_HEAD
#define RAW_RPC_STUB_HEAD

#include <deque>
#include <map>
#include "uvpp.h"
#include "rrpcmessage.h"

class rrpcstub {
	public:
	rrpcstub();
	~rrpcstub();

	virtual void dispatch();
	virtual bool onmessage(char* msg) = 0;
	void pushmessage(char* buff, int len);

	bool resumecall(short sid);
	bool syncmessage(short sid, char* msg);
	// caller should free returned memory
	char* call(uvpp::client& c, RRpcMessage& msg, unsigned short timeout=5);

	short nextsid() {
		_sidmtx.lock();
		_session++;
		_sidmtx.unlock();
		return _session;
	}

	private:
	char uncomp[65535];
	unsigned short unsize;

	std::deque<char*> _compque;
	uvpp::mutex _comtx;
 	uvpp::condition _comcond;

 	uvpp::mutex _syncmtx;
	std::map<short, uvpp::condition*> _syncond;
	std::map<short, char*> _syncmsg;

	short _session;
	uvpp::mutex _sidmtx;
};

#endif
