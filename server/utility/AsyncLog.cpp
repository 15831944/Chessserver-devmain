#include "AsyncLog.h"
#include <ctime>
#include <cstdio>

void async_log_print_loop_run(void* arg) {
	AsyncLog* log = static_cast<AsyncLog*>(arg);
	log->PrintLog();
}

void AsyncLog::Init(const char* path, const char* prefix, int level) {
	lvl = level;
	if (path) {
		SYSTEMTIME SystemTime;
		GetLocalTime(&SystemTime);

		_snprintf_s(fprefix, sizeof(fprefix), _TRUNCATE, "%s/%s", path, prefix);
		_snprintf_s(curfile, sizeof(curfile), _TRUNCATE, "%s%02d%02d%02d.%d.log", fprefix, SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, pid);
		if (pFile) fclose(pFile);

		pFile = fopen(curfile, "ab+");
		day = (char)SystemTime.wDay;
	}

	if (!selfRun) {
		selfRun = true;
		uv_thread_t tid;
		uv_thread_create(&tid, async_log_print_loop_run, this);
	}
}

void AsyncLog::Log(void* data, size_t size) {
	static char newline[] = {0x0d, 0x0a};
	int count = 0;
	if (pFile) {
		mtxTick.lock();
		for (; count<(int)pedding.size(); ++count) {
			std::string& str = pedding[count];
			fwrite(str.data(), 1, str.size(), pFile);
			fwrite(newline, 1, 2, pFile);
		}
		if (data) {
			fwrite(data, 1, size, pFile);
			fwrite(newline, 1, 2, pFile);
		}
		if (fflush(pFile) != 0) {
			count = pedding.size();
		}
		mtxTick.unlock();
		TickAndSwitch(count+(data?1:0));
	} else {
		for (; count<(int)pedding.size(); ++count) {
			std::string& str = pedding[count];
			printf("%s\n", str.data());
		}
		if (data) printf("%s\n", (char*)data);
	}
	if (count > 0) {
		mtx.lock();
		pedding.erase(pedding.begin(), pedding.begin()+count);
		mtx.unlock();
	}
}

void AsyncLog::Push(void* data, size_t size) {
	std::string buf((const char*)data, size);
	mtx.lock();
	pedding.push_back(buf);
	mtx.unlock();
	cond.post();
}

void AsyncLog::Push(const std::string& data) {
	mtx.lock();
	pedding.push_back(data);
	mtx.unlock();
	cond.post();
}

void AsyncLog::Push(int level, bool precise, const char* func, int line, const char* pszFormat, ...) {
	if (level < lvl) {
		return;
	}
	char tailbuff[2048];
	va_list args;
	va_start(args, pszFormat);
	int tailLen = _vsnprintf_s(tailbuff, sizeof(tailbuff), _TRUNCATE, pszFormat, args);
	va_end(args);

	char prebuff[2048];
	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);
	int preLen = _snprintf_s(prebuff, sizeof(prebuff), _TRUNCATE, "%02d-%02d %02d:%02d:%02d",
		SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond);
	if (precise) {
		preLen += _snprintf_s(prebuff+preLen, sizeof(prebuff)-preLen, _TRUNCATE, ".%03d", SystemTime.wMilliseconds);
	}
	preLen += _snprintf_s(prebuff+preLen, sizeof(prebuff)-preLen, _TRUNCATE, "[%s:%d]", func, line);
	
	std::string str(prebuff, preLen);
	str.append(tailbuff, tailLen);
	Push(str);
}

void AsyncLog::TickAndSwitch(unsigned short incr) {
	static unsigned short seq = 1;
	static unsigned short tick = 0;
	static time_t last = time(0);

	if (!pFile) return ;

	tick += incr;
	time_t now = time(0);
	if (now > (last+120)) { // 日切
		last = now;
		SYSTEMTIME SystemTime;
		GetLocalTime(&SystemTime);
		if (SystemTime.wDay != day) {
			_snprintf_s(curfile, sizeof(curfile), _TRUNCATE, "%s%02d%02d%02d.%d.log", fprefix, SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, pid);
			mtxTick.lock();
			fclose(pFile);
			pFile = fopen(curfile, "ab+");
			mtxTick.unlock();

			day = (char)SystemTime.wDay;
			seq = 1;
			return ;
		}
	}
	if (tick > 2000) { // 预估每写100k数据,检测一遍
		tick = 0;
		if (ftell(pFile) >= 512000000) { // 日志文件达到512M
			char backup[128];
			_snprintf_s(backup, sizeof(backup), _TRUNCATE, "%s.%d", curfile, seq++);
			mtxTick.lock();
			fclose(pFile);
			rename(curfile, backup);
			pFile = fopen(curfile, "ab+");
			mtxTick.unlock();
		}
	}
}
