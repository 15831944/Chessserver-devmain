#ifndef LOGON_RPC_HEAD_FILE
#define LOGON_RPC_HEAD_FILE

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "grpcpp/grpcpp.h"
#include "grpc/support/log.h"
#include "qpserver.grpc.pb.h"
#include "common.rpc.h"
#include "../server/global/AsyncLog.h"

class LogonClient {
  public:
    static LogonClient& Instance() {
      static LogonClient instance;
      return instance;
    }

    void SetChannel(std::shared_ptr<grpc::Channel> channel) {
      stub_ = qpserver::Logon::NewStub(channel);
      if (!asyncRun) {
        asyncRun = true;
        std::thread(&LogonClient::AsyncCompleteRpc, this).detach();
      }
    }

    int SendSms(int kind, const std::string& phone, std::string& out) {
      qpserver::ReqSendSms request;
      request.set_kind(kind);
      request.set_phone(phone);

      qpserver::RepRet reply;
      // Context for the client. It could be used to convey extra information to
      // the server and/or tweak certain RPC behaviors.
      grpc::ClientContext context;
      grpc::Status status = stub_->SendSms(&context, request, &reply);
      out = reply.error();

      RPC_COMMON_CODE_RETURN(status,reply,ERR_SMSFAIL);
    }

    int VerifySms(int kind, const std::string& phone, const std::string& code, bool erase=false) {
      qpserver::ReqVerifySms request;
      request.set_kind(kind);
      request.set_phone(phone);
      request.set_code(code);
      request.set_erase(erase);

      qpserver::RepRet reply;
      grpc::ClientContext context;
      grpc::Status status = stub_->VerifySms(&context, request, &reply);

      RPC_COMMON_CODE_RETURN(status,reply,ERR_VCODEFAIL);
    }

    void EraseSms(int kind, const std::string& phone) {
      qpserver::ReqSendSms request;
      request.set_kind(kind);
      request.set_phone(phone);

      AsyncCall* call = new AsyncCall;
      call->respReader = stub_->PrepareAsyncEraseSms(&call->context, request, &cq_);
      call->respReader->StartCall();
      call->respReader->Finish(&call->reply, &call->status, (void*)call);
      call->func = __func__;
    }

    int RegPreVerify(const std::string& phone, const std::string& code, const std::string& nickname) {
      qpserver::ReqRegVerifyItem request;
      request.set_phone(phone);
      request.set_code(code);
      request.set_nickname(nickname);

      qpserver::RepRet reply;
      grpc::ClientContext context;
      grpc::Status status = stub_->RegPreVerify(&context, request, &reply);

      RPC_COMMON_CODE_RETURN(status,reply,ERR_VCODEFAIL);
    }

    void UpdateUser(int uid, int spid, int rebate, const std::string& phone, const std::string& machine, const std::string& nick) {
      qpserver::ReqUpdUser request;
      request.set_user(uid);
      request.set_spreader(spid);
      request.set_rebate(rebate);
      request.set_phone(phone);
      request.set_machine(machine);
      request.set_nickname(nick);

      AsyncCall* call = new AsyncCall;
      call->respReader = stub_->PrepareAsyncUpdateUser(&call->context, request, &cq_);
      call->respReader->StartCall();
      call->respReader->Finish(&call->reply, &call->status, (void*)call);
      call->func = __func__;
    }

    void MapUser(int uid, const std::string& phone, const std::string& third) {
      qpserver::ReqMapUser request;
      request.set_user(uid);
      request.set_phone(phone);
      request.set_third(third);

      AsyncCall* call = new AsyncCall;
      call->respReader = stub_->PrepareAsyncMapUser(&call->context, request, &cq_);
      call->respReader->StartCall();
      call->respReader->Finish(&call->reply, &call->status, (void*)call);
      call->func = __func__;
    }

 private:
  LogonClient():asyncRun(false) {}
  // prevent copies
  LogonClient(LogonClient const&);
  void operator=(LogonClient const&);

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

  std::unique_ptr<qpserver::Logon::Stub> stub_;
  grpc::CompletionQueue cq_;
  bool asyncRun;
};


#define REFRPC_LOGON(value) REF_G(LogonClient, value)

#endif