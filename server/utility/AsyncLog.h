#ifndef ASYNC_LOG_HEAD
#define ASYNC_LOG_HEAD

#include <deque>
#include "uvpp.h"

class AsyncLog {
	public:
	static AsyncLog& Instance() {
		static AsyncLog instance;
		return instance;
	}

	void Init(const char* path, const char* prefix, int level=0);
	void Log(void* data, size_t size);
	void Push(void* data, size_t size);
	void Push(const std::string& data);
	void Push(int level, bool precise, const char* func, int line, const char* pszFormat, ...);
	void PrintLog() {
		do {
			while (pedding.size() == 0) cond.wait(5*CONDSEC);

			Log(0, 0);
		} while(true);
	}

	private:
	AsyncLog():pFile(0), selfRun(false), lvl(0) {
		pid = GetCurrentProcessId();
	}
	~AsyncLog() {
		if (pFile) fclose(pFile);
	}

	// prevent copies
	AsyncLog(AsyncLog const&);
	void operator=(AsyncLog const&);

	void TickAndSwitch(unsigned short incr);

	char day;
	char curfile[128];
	char fprefix[128];
	FILE* pFile;
	int pid;
	int lvl;

	bool selfRun;
	std::deque<std::string> pedding;
	uvpp::mutex mtx;
	uvpp::mutex mtxTick;
	uvpp::condition cond;
};

#define ALDeb(format, ...)\
{\
	REF_G(AsyncLog, Log);\
	Log.Push(0, false, __FUNCTION__, __LINE__, format, __VA_ARGS__); \
}

#define ALStd(format, ...)\
{\
	REF_G(AsyncLog, Log);\
	Log.Push(1, false, __FUNCTION__, __LINE__, format, __VA_ARGS__); \
}

#define ALErr(format, ...)\
{\
	REF_G(AsyncLog, Log);\
	Log.Push(2, false, __FUNCTION__, __LINE__, format, __VA_ARGS__); \
}

#define ModuleLogInit(tag) \
{\
	wchar_t logPath[128] = {0}; \
	wchar_t logLvl[128] = {0}; \
	CFG_READ_STR(tag,Log,"std",logPath); \
	CFG_READ_STR(tag,LogLvl,"0",logLvl); \
	REF_G(AsyncLog, Log); \
	if (wcscmp(logPath, L"std") == 0) { \
		DebugLog("%s Server init", #tag); \
		Log.Init(0, 0, _wtoi(logLvl)); \
	} else { \
		UNI2UTF_S(logPath, pPath); \
		Log.Init(pPath, #tag, _wtoi(logLvl)); \
		ALErr("%s Server init", #tag); \
	}\
}

#endif
