#ifndef COMMON_RPC_HEAD_FILE
#define COMMON_RPC_HEAD_FILE

#include <unordered_map>

#define RPC_COMMON_CODE_RETURN(status,reply,deferr) \
if (status.ok()) return reply.code();\
{ PLOG("RPC FAILED:%d:%s", status.error_code(), status.error_message().data()); } \
return (RpcReply::deferr);

#define RPC_CODEMSG(code,msg) const std::string& msg = RpcReply::MapRetMsg.at((code))

namespace RpcReply {
  const unsigned short ERR_OK = 0;
  const unsigned short ERR_FAIL = 1;
  const unsigned short ERR_OPERMAX = 2;
  const unsigned short ERR_SESSION = 3;
  const unsigned short ERR_SMSOK = 1000;
  const unsigned short ERR_SMSFAIL = 1001;
  const unsigned short ERR_MBACCEXISTS = 1002;
  const unsigned short ERR_PHONE = 1003;
  const unsigned short ERR_NOMBACC = 1004;
  const unsigned short ERR_VCODEOK = 1100;
  const unsigned short ERR_VCODEFAIL = 1101;
  const unsigned short ERR_VCODEXPIRE = 1102;
  const unsigned short ERR_VCODEWRONG = 1103;
  const unsigned short ERR_VNICKNAME = 1104;
  const unsigned short ERR_MAPUSEROK = 1200;
  const unsigned short ERR_MAPUSERFAIL = 1201;
  const unsigned short ERR_REMAPUSER = 1202;
  const unsigned short ERR_UPDUSEROK = 1300;
  const unsigned short ERR_UPDUSERFAIL = 1301;

  const std::unordered_map<unsigned short, std::string> MapRetMsg = {
    {ERR_OK, u8"OK"},
    {ERR_FAIL, u8"调用失败了"},
    {ERR_OPERMAX, u8"您的操作次数过多, 请稍后尝试"},
    {ERR_SESSION, u8"SESSION不存在"},
    {ERR_SMSOK, u8"验证码已发送"},
    {ERR_SMSFAIL, u8"短信发送失败了"},
    {ERR_MBACCEXISTS, u8"手机号码已注册"},
    {ERR_PHONE, u8"手机号码错误"},
    {ERR_NOMBACC, u8"您输入的手机号码还未注册账户"},
    {ERR_VCODEOK, u8"验证通过"},
    {ERR_VCODEFAIL, u8"验证失败了"},
    {ERR_VCODEXPIRE, u8"您的验证码已失效"},
    {ERR_VCODEWRONG, u8"验证码错误"},
    {ERR_VNICKNAME, u8"昵称已被使用了"},
    {ERR_MAPUSEROK, u8"映射用户成功"},
    {ERR_MAPUSERFAIL, u8"映射用户失败了"},
    {ERR_REMAPUSER, u8"重复映射用户"},
    {ERR_UPDUSEROK, u8"更新用户成功"},
    {ERR_UPDUSERFAIL, u8"更新用户失败"},
  };
};

#endif
