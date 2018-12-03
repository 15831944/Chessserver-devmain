#ifndef GAME_RPC_HEAD_FILE
#define GAME_RPC_HEAD_FILE

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "grpcpp/grpcpp.h"
#include "grpc/support/log.h"
#include "qpserver.grpc.pb.h"
#include "common.rpc.h"
#include "../server/global/AsyncLog.h"

class GameClient {
  public:
  static GameClient& Instance() {
    static GameClient instance;
    return instance;
  }

  void SetChannel(std::shared_ptr<grpc::Channel> channel) {
    stub_ = qpserver::Game::NewStub(channel);
    if (!asyncRun) {
      asyncRun = true;
      std::thread(&GameClient::AsyncCompleteRpc, this).detach();
    }
  }

  void GamePlayStatis(int user, int game, int face, const std::string& nickname, int playtime, long long vscore, long long tax ) {
    qpserver::ReqInOutGame request;
    request.set_user(user);
    request.set_game(game);
    request.set_face(face);
    request.set_nickname(nickname);
    request.set_playtime(playtime);
    request.set_vscore(vscore);
    request.set_tax(tax);

    AsyncCall* call = new AsyncCall;
    call->respReader = stub_->PrepareAsyncGamePlayStatis(&call->context, request, &cq_);
    call->respReader->StartCall();
    call->respReader->Finish(&call->reply, &call->status, (void*)call);
    call->func = __func__;
  }

  private:
  GameClient():asyncRun(false) {}
  // prevent copies
  GameClient(GameClient const&);
  void operator=(GameClient const&);

  struct AsyncCall {
    AsyncCall():func(0){}

    qpserver::RepRet reply;
    grpc::ClientContext context;
    grpc::Status status;
    std::unique_ptr<grpc::ClientAsyncResponseReader<qpserver::RepRet>> respReader;
    void* func;
  };

  void AsyncCompleteRpc() {
    void* got_tag;
    bool ok = false;

    // Block until the next result is available in the completion queue "cq".
    while (cq_.Next(&got_tag, &ok)) {
        // The tag in this example is the memory location of the call object
        AsyncCall* call = static_cast<AsyncCall*>(got_tag);

        // Verify that the request was completed successfully. Note that "ok"
        // corresponds solely to the request for updates introduced by Finish().
        GPR_ASSERT(ok);
        if (!call->status.ok()) {
          PLOG("Async %s failed %d:%s", call->func ? call->func:"?", call->status.error_code(), call->status.error_message().data());
        } else {
          PLOG("Async %s Disgard %d:%s", call->func ? call->func:"?", call->reply.code(), call->reply.error().data());
        }
        delete call;
    }
  }

  std::unique_ptr<qpserver::Game::Stub> stub_;
  grpc::CompletionQueue cq_;
  bool asyncRun;
};


#define REFRPC_GAME(value) REF_G(GameClient, value)

#endif