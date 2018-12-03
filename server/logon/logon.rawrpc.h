#ifndef LOGON_RAW_RPC_HEAD
#define LOGON_RAW_RPC_HEAD

#include <deque>
#include "../utility/uvpp.h"
#include "../utility/rrpcmessage.h"
#include "../utility/rrpcstub.h"
#include "../utility/AsyncLog.h"
#include "../proto/rrpcproto.h"

class LogonRRpc: public uvpp::itcpclient, public rrpcstub {
	public:
	static LogonRRpc& Instance() {
		static LogonRRpc instance;
		return instance;
	}

	void Connect(const char* address) {
		const char* sep = strchr(address, ':');
		memset(host, 0, sizeof(host));
		memcpy(host, address, sep-address);
		port = (unsigned short)atoi(sep+1);

		client.connect(host, port);
		semconn.wait();
	}

	void Connect(const char* host, unsigned short port) {
		strcpy_s(this->host, host);
		this->port = port;
		client.connect(host, port);
		semconn.wait();
	}

	/////////////////////////////////////////////////////////////////////////////////////
	int SendSms(char kind, const char* phone, std::string& out) {
    RRpcMessage* msg = msgpool.get();
		msg->Reset();
		msg->Push16(RRPC_CMD_SENDSMS);
    msg->Push8(kind);
    msg->PushBytes(phone, strlen(phone));
    msg->Final(true, nextsid());

    char* reply = call(client, *msg);
		msgpool.put(msg);
		if (!reply) {
			return RRPC_ERR_SMSFAIL;
		}
		RRpcMsgSafeWrap(reply, wrap);
		if (wrap->size >= 2) {
			RRpcProtoReader reader(wrap->data);

			reader.Bytes(4, out);
			return reader.I16(2); //skip cmd
		}
    return RRPC_ERR_SMSFAIL;
  }

	int VerifySms(char kind, const char* phone, const char* code, bool erase=false) {
		RRpcMessage* msg = msgpool.get();
		msg->Reset();
		msg->Push16(RRPC_CMD_VERIFYSMS);
		msg->Push8(kind);
		msg->PushBytes(phone, strlen(phone));
		msg->PushBytes(code, strlen(code));
		msg->Push8(erase ? 1:0);
		msg->Final(true, nextsid());

		char* reply = call(client, *msg);
		msgpool.put(msg);
		RRPC_REPLY_CODE(reply, RRPC_ERR_VCODEFAIL);
  }

  int RegPreVerify(const char* phone, const char* code, const char* nickname) {
    RRpcMessage* msg = msgpool.get();
		msg->Reset();
		msg->Push16(RRPC_CMD_REGVERIFY);
		msg->PushBytes(phone, strlen(phone));
		msg->PushBytes(code, strlen(code));
		msg->PushBytes(nickname, strlen(nickname));
		msg->Final(true, nextsid());

		char* reply = call(client, *msg);
		msgpool.put(msg);
		RRPC_REPLY_CODE(reply, RRPC_ERR_VCODEFAIL);
  }

  std::string IPSpreadCode(const char* ip) {
  	RRpcMessage* msg = msgpool.get();
		msg->Reset();
		msg->Push16(RRPC_CMD_IPSPREADCODE);
		msg->PushBytes(ip, strlen(ip));
		msg->Final(true, nextsid());

		char* reply = call(client, *msg);
		msgpool.put(msg);
		RRPC_REPLY_MSG(reply);
  }

  bool UpdateUser(int uid, int spid, short rebate, char agent, const char* phone, const char* machine, const char* nick, int gameid) {
    RRpcMessage* msg = msgpool.get();
		msg->Reset();
		msg->Push16(RRPC_CMD_UPDATEUSER);
		msg->Push32(uid);
		msg->Push32(spid);
		msg->Push16(rebate);
		msg->Push8(agent);
		msg->PushBytes(phone, strlen(phone));
		msg->PushBytes(machine, strlen(machine));
		msg->PushBytes(nick, strlen(nick));
		msg->Push32(gameid);
		msg->Final(true, nextsid());

		char* reply = call(client, *msg);
		msgpool.put(msg);
		if (!reply) {
			return false;
		}
		RRpcMsgSafeWrap(reply, wrap);
		if (wrap->size >= 4) {
			RRpcProtoReader reader(wrap->data);
			return reader.I16(2) == RRPC_ERR_UPDUSEROK;
		}
		return false;
  }

  void EraseSms(int kind, const char* phone) {
  	RRpcMessage* msg = msgpool.get();
		msg->Reset();
		msg->Push16(RRPC_CMD_ERASESMS);
		msg->Push8(kind);
		msg->PushBytes(phone, strlen(phone));
		msg->Final(true, nextsid());
		client.send(msg->data, msg->size, (void*)msg);
  }

  void UserStatistic(int uid, int64 registed, int64 binded, int64 present) {
  	RRpcMessage* msg = msgpool.get();
		msg->Reset();
		msg->Push16(RRPC_CMD_USERSATISTIC);
		msg->Push32(uid);
		msg->Push64(registed);
		msg->Push64(binded);
		msg->Push64(present);
		msg->Final(true, nextsid());
		client.send(msg->data, msg->size, (void*)msg);
  }

  /////////////////////////////////////////////////////////////////////////////////////
	virtual void onconnect(int status) {
		semconn.post();
		if (status != 0) {
			ALErr("Connect to rpc failed %d", status);
		} else {
			int n = sendfails.count();
			ALErr("Rpc service connected, padding msg:%d", n);
			for (; n>0; --n) {
				RRpcMessage* msg = sendfails.get();
				client.send(msg->data, msg->size, (void*)msg);
			}
		}
	}

	virtual bool onmessage(char* msg) {
		RRpcMessageWrapper* wrap = (RRpcMessageWrapper*)msg;
		if (wrap->Size() > 0) {
			RRpcProtoReader proto(wrap->data);
			short cmd = proto.N16();
			ALStd("Rpc reply[%d] cmd:0x%02hx", wrap->Sid(), cmd);
			// send rpc reply handle here
			switch(cmd) {
				case RRPC_CMD_ERASESMS:
				case RRPC_CMD_USERSATISTIC: {

				} return true;
			}
		}
		// call rpc reply handle
		return syncmessage(wrap->Sid(), msg);
	}

	virtual void onread(char* buff, ssize_t len) {
		if (buff) {
			pushmessage(buff, len);
		}
	}

	virtual void afterwrite(void* arg, int status) {
		RRpcMessage* msg = (RRpcMessage*)arg;
		if (msg->iscall) {
			if (status != 0) {
				resumecall(msg->sid);
			}
		} else {
			if (status != 0) {
				sendfails.put(msg);
			} else {
				msgpool.put(msg);
			}
		}
	}

	private:
	LogonRRpc():client(this),semconn(0),
		timer(10000, 10000, &LogonRRpc::oncheck, this) {
	}

	void oncheck() {
		if (!client.iswritable()) {
			ALErr("Connection not writabel! padding:%d", sendfails.count());
			client.connect(host, port);
		}
	}

	uvpp::objpool<RRpcMessage> sendfails;
	uvpp::objpool<RRpcMessage> msgpool;

	uvpp::client client;
	uvpp::semaphore semconn;
	uvpp::timer<LogonRRpc> timer;

	char host[32];
	unsigned short port;
};

#define RPCLOGON(value) REF_G(LogonRRpc, value)

#endif
