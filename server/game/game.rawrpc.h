#ifndef GAME_RAW_RPC_HEAD
#define GAME_RAW_RPC_HEAD

#include <deque>
#include "../utility/uvpp.h"
#include "../utility/rrpcmessage.h"
#include "../utility/rrpcstub.h"
#include "../utility/AsyncLog.h"
#include "../proto/rrpcproto.h"

class GameRRpc: public uvpp::itcpclient, public rrpcstub {
	public:
	static GameRRpc& Instance() {
		static GameRRpc instance;
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
		strcpy(this->host, host);
		this->port = port;
		client.connect(host, port);
		semconn.wait();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////
	void GamePlayStatis(int user, short game, short face, int playtime, int vscore, const char* nickname) {
		ALDeb("user %d in game %d play:%d %d", user, game, playtime, vscore);
		RRpcMessage* msg = msgpool.get();
		msg->Reset();
		msg->Push16(RRPC_CMD_GAMEPLAYSTATIS);
		msg->Push32(user);
		msg->Push16(game);
		msg->Push16(face);
		msg->Push32(playtime);
		msg->Push32(vscore);
		msg->PushBytes(nickname, strlen(nickname));
		msg->Final(true, nextsid());
		client.send(msg->data, msg->size, (void*)msg);
	}

	void UpdateTrumpetScore() {
		RRpcMessage* msg = msgpool.get();
		msg->Reset();
		msg->Push16(RRPC_CMD_TRUMPETSCORE);
	  msg->Final(true, nextsid());

	  char* reply = call(client, *msg);
		msgpool.put(msg);
		if (!reply) {
			return ;
		}
		RRpcMsgSafeWrap(reply, wrap);
		if (wrap->size >= 6) {
			RRpcProtoReader reader(wrap->data);
			int score = reader.I32(2); //skip cmd
			if (score > 0) {
				mTrumpetScore = score;
			}
		}
	}

	void GameSaleStatis(int user, int64 sales, int64 taxes, int64 drawid) {
		ALDeb("user %d sales %d taxes %d %d", user, sales, taxes, drawid);
		RRpcMessage* msg = msgpool.get();
		msg->Reset();
		msg->Push16(RRPC_CMD_SALESTATIS);
		msg->Push32(user);
		msg->Push64(sales);
		msg->Push64(taxes);
		msg->Push64(drawid);
		msg->Final(true, nextsid());
		client.send(msg->data, msg->size, (void*)msg);
	}

	int mTrumpetScore;

  //////////////////////////////////////////////////////////////////////////////////////////////////////////
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
			switch(cmd) {
				case RRPC_CMD_GAMEPLAYSTATIS:
				case RRPC_CMD_SALESTATIS: {

				} return true;
			}
		}
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
	GameRRpc():client(this),semconn(0),
		timer(15000, 15000, &GameRRpc::oncheck, this) {
		mTrumpetScore = 100000;
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

	uvpp::timer<GameRRpc> timer;

	char host[32];
	unsigned short port;
};

#define RPCGAME(value) REF_G(GameRRpc, value)

#endif
