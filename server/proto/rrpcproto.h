#ifndef RAW_RPC_PROTO_HEAD
#define RAW_RPC_PROTO_HEAD

const short RRPC_CMD_REPLY = 0x0000;
const short RRPC_CMD_GAMEPLAYSTATIS = 0x1000;
const short RRPC_CMD_TRUMPETSCORE = 0x1001;
const short RRPC_CMD_SALESTATIS = 0x1002;
const short RRPC_CMD_SENDSMS = 0x2000;
const short RRPC_CMD_VERIFYSMS = 0x2001;
const short RRPC_CMD_REGVERIFY = 0x2002;
const short RRPC_CMD_IPSPREADCODE = 0x2003;
const short RRPC_CMD_UPDATEUSER = 0x2004;
const short RRPC_CMD_ERASESMS = 0x2005;
const short RRPC_CMD_USERSATISTIC = 0x2006;

const unsigned short RRPC_ERR_OK = 0;
const unsigned short RRPC_ERR_FAIL = 1;
const unsigned short RRPC_ERR_OPERMAX = 2;
const unsigned short RRPC_ERR_SESSION = 3;
const unsigned short RRPC_ERR_SMSOK = 1000;
const unsigned short RRPC_ERR_SMSFAIL = 1001;
const unsigned short RRPC_ERR_MBACCEXISTS = 1002;
const unsigned short RRPC_ERR_PHONE = 1003;
const unsigned short RRPC_ERR_NOMBACC = 1004;
const unsigned short RRPC_ERR_VCODEOK = 1100;
const unsigned short RRPC_ERR_VCODEFAIL = 1101;
const unsigned short RRPC_ERR_VCODEXPIRE = 1102;
const unsigned short RRPC_ERR_VCODEWRONG = 1103;
const unsigned short RRPC_ERR_VNICKNAME = 1104;
const unsigned short RRPC_ERR_MAPUSEROK = 1200;
const unsigned short RRPC_ERR_MAPUSERFAIL = 1201;
const unsigned short RRPC_ERR_REMAPUSER = 1202;
const unsigned short RRPC_ERR_UPDUSEROK = 1300;
const unsigned short RRPC_ERR_UPDUSERFAIL = 1301;

static const char* RrpcErrorString(unsigned short code) {
	static std::map<unsigned short, char*> means;
	if (means.size() == 0) {
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_OK, UnicodeToUtf8(L"OK")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_FAIL, UnicodeToUtf8(L"调用失败了")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_OPERMAX, UnicodeToUtf8(L"您的操作次数过多, 请稍后尝试")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_SESSION, UnicodeToUtf8(L"SESSION不存在")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_SMSOK, UnicodeToUtf8(L"验证码已发送")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_SMSFAIL, UnicodeToUtf8(L"短信发送失败了")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_MBACCEXISTS, UnicodeToUtf8(L"手机号码已注册")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_PHONE, UnicodeToUtf8(L"手机号码错误")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_NOMBACC, UnicodeToUtf8(L"您输入的手机号码还未注册账户")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_VCODEOK, UnicodeToUtf8(L"验证通过")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_VCODEFAIL, UnicodeToUtf8(L"验证失败了")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_VCODEXPIRE, UnicodeToUtf8(L"您的验证码已失效")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_VCODEWRONG, UnicodeToUtf8(L"验证码错误")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_VNICKNAME, UnicodeToUtf8(L"昵称已被使用了")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_MAPUSEROK, UnicodeToUtf8(L"映射用户成功")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_MAPUSERFAIL, UnicodeToUtf8(L"映射用户失败了")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_REMAPUSER, UnicodeToUtf8(L"重复映射用户")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_UPDUSEROK, UnicodeToUtf8(L"更新用户成功")));
		means.insert(std::pair<unsigned short, char*>(RRPC_ERR_UPDUSERFAIL, UnicodeToUtf8(L"更新用户失败")));
	}
	std::map<unsigned short, char*>::iterator it = means.find(code);
	return it != means.end() ? it->second:"";
}

#define RRPC_REPLY_CODE(reply, def) \
if (!reply) { return def; } \
RRpcMsgSafeWrap(reply, wrap); \
if (wrap->size >= 4) { \
	RRpcProtoReader reader(wrap->data); \
	return reader.I16(2); \
} \
return def

#define RRPC_REPLY_MSG(reply) \
if (!reply) { return ""; } \
RRpcMsgSafeWrap(reply, wrap); \
if (wrap->size >= 6) { \
	RRpcProtoReader reader(wrap->data); \
	std::string reply##_msg; \
	reader.Bytes(4, reply##_msg); \
	return reply##_msg; \
} \
return ""

#endif
