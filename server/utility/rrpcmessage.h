#ifndef RAW_RPC_MESSAGE_HEAD
#define RAW_RPC_MESSAGE_HEAD

typedef long long int64;

#define Swap8Bytes(val) \
( (((val) >> 56) & 0x00000000000000FF) | (((val) >> 40) & 0x000000000000FF00) | \
   (((val) >> 24) & 0x0000000000FF0000) | (((val) >>  8) & 0x00000000FF000000) | \
   (((val) <<  8) & 0x000000FF00000000) | (((val) << 24) & 0x0000FF0000000000) | \
   (((val) << 40) & 0x00FF000000000000) | (((val) << 56) & 0xFF00000000000000) )

static bool LTENDIAN() {
	static union {
		char _c[2];
		short _s;
	} _t{{'L','B'}};
	return (char)(_t._s) == 'L';
}

static int64 htonll(int64 val) {
	if (LTENDIAN()) return Swap8Bytes(val);
	return val;
}

#define ntohll htonll

/*	HEAD:
		LEN - 2 bytes package len, not inlcude HEAD
		CTL - 2 bytes control infomation:
			FIN - 1 bit, indicate multiple packages, 0 means finished
			SID - 15 bit, use for synchronise
		DATA: LEN bytes of data
*/
const size_t DATAINCR = 512;

struct RRpcMessage {
	RRpcMessage():data(0),iscall(false) {
		Extend();
		size = 4;
		memset(data, 0, size);
	}

	~RRpcMessage() {
		if (data) {
			free(data);
		}
	}

	void Reset() {
		size = 4;
		iscall = false;
		memset(data, 0, size);
	}

	void Extend(size_t incr=DATAINCR) {
		size_t actincr = DATAINCR;
		while( actincr < incr ) actincr *= 2;

		if (!data) {
			data = (char*)malloc(actincr);
			size = 0;
			cap = actincr;
		} else {
			char* old = data;
			cap += actincr;
			data = (char*)malloc(cap);
			memcpy(data, old, size);
			free(old);
		}
	}

	void Final(bool fin, short sid) {
		this->sid = sid;
		*(short*)(data) = htons((short)(size-4));
		*(short*)(data+2) = htons((short)(fin ? 0x0000:0x8000)|(0x7FFF&sid));
	}

	size_t Push8(char value) {
		if ((size + 1) > cap) {
			Extend(1);
		}
		*(char*)(data+size) = value;
		size += 1;
		return size;
	}

	size_t Push16(short value) {
		if ((size + 2) > cap) {
			Extend(2);
		}
		*(short*)(data+size) = htons(value);
		size += 2;
		return size;
	}

	size_t Push32(int value) {
		if ((size + 4) > cap) {
			Extend(4);
		}
		*(int*)(data+size) = htonl(value);
		size += 4;
		return size;
	}

	size_t Push64(int64 value) {
		if ((size + 8) > cap) {
			Extend(8);
		}
		*(int64*)(data+size) = htonll(value);
		size += 8;
		return size;
	}

	size_t PushBytes(const char* buff, size_t len) {
		Push16(len < 1 ? 0:len);
		if (len < 1) {
			return 0;
		}
		if ((size + len) > cap) {
			Extend(len);
		}
		memcpy(data+size, buff, len);
		size += len;
		return size;
	}

	char* data;
	size_t size;
	size_t cap;
	short sid;
	bool iscall;
};

struct RRpcMessageWrapper {
	unsigned short size;
	unsigned short control;
	char data[1];

	unsigned short Size() {
		return ntohs(size);
	}

	bool Fin() {
		return (short)((0x8000)&ntohs(control)) == 0;
	}

	short Sid() {
		return (short)(0x7FFF)&ntohs(control);
	}
};

struct RRpcProtoReader {
	RRpcProtoReader(char* ref):data(ref),rpos(0) {}

	short I16(int pos) {
		return ntohs(*(short*)(data+pos));
	}

	int I32(int pos) {
		return ntohl(*(int*)(data+pos));
	}

	void Bytes(int pos, std::string& dest) {
		unsigned short len = I16(pos);
		dest.assign(data+pos+2, len);
	}

	short N16() {
		short value = I16(rpos);
		rpos += 2;
		return value;
	}

	int N32() {
		int value = I32(rpos);
		rpos += 4;
		return value;
	}

	void NBytes(std::string& dest) {
		Bytes(rpos, dest);
		rpos += dest.size() + 2;
	}

	char* data;
	int rpos;
};

#define RRpcMsgSafeWrap(src, dst) std::unique_ptr<char[],std::default_delete<char[]>> _safe_##src(src);\
RRpcMessageWrapper* dst = (RRpcMessageWrapper*)src;

#endif
