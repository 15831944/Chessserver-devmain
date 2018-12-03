﻿#pragma once
#include "stdio.h"
#include <stdarg.h>    ////用变参函数所必须的 
#include <time.h>
#include <direct.h> 
#include <mmsystem.h>
#include <memory>
#pragma comment(lib, "winmm.lib")

// 将宽字节wchar_t* 转换 单字节char*
static char* UnicodeToAnsi( const wchar_t* szStr )
{
	int nLen = WideCharToMultiByte( CP_ACP, 0, szStr, -1, NULL, 0, NULL, NULL );
	if (nLen == 0)
	{
		return NULL;
	}
	char* pResult = new char[nLen];
	WideCharToMultiByte( CP_ACP, 0, szStr, -1, pResult, nLen, NULL, NULL );
	return pResult;
}

// 将单字节char* 转换 宽字节wchar_t*
static wchar_t* AnsiToUnicode( const char* szStr )
{
	wchar_t* pWCHAR=NULL;  
  
	//计算pChar所指向的多字节字符串相当于多少个宽字节  
	DWORD num=MultiByteToWideChar(CP_ACP,0,szStr,-1,NULL,0);  
  
	pWCHAR=(wchar_t*)malloc(num*sizeof(wchar_t));  
  
	if (pWCHAR==NULL)  
	{  
		free(pWCHAR);  
	}  
  
	memset(pWCHAR,0,num*sizeof(wchar_t));  
  
	//多字节转换为宽字节  
	MultiByteToWideChar(CP_ACP,0,szStr,-1,pWCHAR,num);  

	return pWCHAR;
}

// 将单字节char* 转换 宽字节wchar_t*
static void AnsiToUnicode( const char* szStr , wchar_t *szDes)
{
	if (NULL == szDes) return;

	//计算pChar所指向的多字节字符串相当于多少个宽字节  
	DWORD num=MultiByteToWideChar(CP_ACP,0,szStr,-1,NULL,0);
  
	//多字节转换为宽字节  
	MultiByteToWideChar(CP_ACP,0,szStr,-1,szDes,num);
}

static char* UnicodeToUtf8( const wchar_t* szStr )
{
	int nLen = WideCharToMultiByte(CP_UTF8, 0, szStr, -1, NULL, 0, NULL, NULL);
	if (nLen == 0)
	{
		return NULL;
	}
	char* pResult = new char[nLen];
	WideCharToMultiByte(CP_UTF8, 0, szStr, -1, pResult, nLen, NULL, NULL);
	return pResult;
}

static void Utf8ToUnicode( const char* szStr , wchar_t *szDes)
{
	if (NULL == szDes) return;

	//计算pChar所指向的多字节字符串相当于多少个宽字节  
	DWORD num=MultiByteToWideChar(CP_UTF8,0,szStr,-1,NULL,0);
  
	//多字节转换为宽字节
	MultiByteToWideChar(CP_UTF8,0,szStr,-1,szDes,num);
}

static void DebugPrintf(const char* pszFormat, ...)
{
	char sLog[1024] = {0};
	va_list args; 
	va_start(args, pszFormat); 
	_vsnprintf(sLog, 1024, pszFormat, args); 
	va_end(args); 
	OutputDebugStringA(sLog);
}

static void LogPrintf( const char* pszFormat, ...) 
//void LogPrintf( const WCHAR* pszFormat, ...) 
{ 
	//char *pTmpBuf = UnicodeToAnsi(pszFormat);

	char sLog[1024] = {0};
	va_list args; 
	va_start(args, pszFormat); 
	_vsnprintf(sLog, 1024, pszFormat, args); 
	va_end(args); 

	//va_start(args, pTmpBuf); 
	//_vsnprintf(sLog, 1024, pTmpBuf, args);
	//va_end(args); 


	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);
	char sInfo[1024] = {0};
	sprintf(sInfo, "open *** 日志信息 %s ", sLog);
	//OutputDebugStringA(sInfo);
	//DebugPrintf(sLog);

	sprintf(sInfo, "%d-%d-%d-%d: %s \r\n",sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sLog );
	int nStatus;

	//创建log文件夹
	nStatus = _mkdir("Log"); 

	char szPath[MAX_PATH] = {0};
	sprintf(szPath, "Log//%d-%d-%d-ServerLog.txt", sysTime.wYear, sysTime.wDay, sysTime.wHour);

	DebugPrintf("openfile  %s",  szPath);

	//以追加的方式打开log文件夹下的ServerLog.txt 文件，如果此文件不存在则创建
	FILE *fp;
	fp = NULL;
	fp = fopen(szPath, "at+");
	if (fp)
	{
		fwrite(sInfo, strlen(sInfo), 1 , fp);
		
		DebugPrintf("openfile 打开文件成功");
	}
	else
	{
		DebugPrintf("openfile 打开文件错误");
	}
	fclose(fp);
}

static void WriteLog( TCHAR * pText, ... )
{
	
	// 信息缓冲
	TCHAR szBuffer[1024] = { _T("") };

	// 转换信息
	va_list VaList;	
	va_start(VaList,pText);
	_vstprintf_s(szBuffer, pText, VaList);	
	va_end(VaList);			

	// 打开文件
	SYSTEMTIME sysTime;
	GetLocalTime(&sysTime);

	//创建log文件夹
	int nStatus = _mkdir("Log"); 

	char szPath[MAX_PATH] = {0};
	sprintf(szPath, "Log//%d-%d-%d-Log.txt", sysTime.wYear, sysTime.wDay, sysTime.wHour);

	FILE *pFile = NULL;
	pFile = fopen(szPath, "at+");
	if(pFile == NULL) return;

	// 变换信息
	//CW2AEX<1024 * 2> szBufferA(szBuffer);
	char* pResult = UnicodeToAnsi(szBuffer);

	// 获取时间
	SYSTEMTIME SystemTime;	
	GetLocalTime( &SystemTime );	
	fprintf( pFile, "%04d-%02d-%02d %02d:%02d:%02d：%s \n",  SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,  SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond,pResult );	
	fflush( pFile ); 
	fclose( pFile ); 
	delete pResult;
	pResult = NULL;
}

// debug log
static void _DebugLog(const char* func, int line, const char* pszFormat, ...)
{
	static BOOL con = false;
	static FILE* fp = 0;
	if (!con) {
		con = AllocConsole();
		freopen_s(&fp, "CONOUT$", "w", stdout);
		SetConsoleOutputCP(65001); //UTF8
		// Remove close button
		HWND handle = GetConsoleWindow();
		HMENU hMenu = GetSystemMenu(handle, false);
		// DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
    EnableMenuItem(hMenu, SC_CLOSE, MF_GRAYED);
	}
	char sLog[2048];
	va_list args; 
	va_start(args, pszFormat);
	int ret = _vsnprintf_s(sLog, sizeof(sLog), _TRUNCATE, pszFormat, args); 
	va_end(args);

	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);
	printf("%02d-%02d %02d:%02d:%02d:[%s:%d]%s %s\n",
		SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond,
		func, line,
		ret < 0 ? "T]":"", sLog);
}

#define DebugLog(format, ...) _DebugLog(__FUNCTION__, __LINE__, format, __VA_ARGS__)

// debug log
static void DebugLogW(const wchar_t* pszFormat, ...)
{
	wchar_t sLog[4096];
	va_list args;
	va_start(args, pszFormat);
	int ret = vswprintf_s(sLog, sizeof(sLog), pszFormat, args);
	va_end(args);

	char* pLog = UnicodeToAnsi(sLog);
	if (!pLog) {
		printf("convert form u to a error");
		return ;
	}

	SYSTEMTIME SystemTime;
	GetLocalTime(&SystemTime);
	printf("%02d-%02d %02d:%02d:%02d:%s %s\n", SystemTime.wMonth, SystemTime.wDay, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, ret < 0 ? "T]":"", pLog);
	delete [] pLog;
}

#define UNI2ANC_S(s,d) char*d=UnicodeToAnsi((s));std::unique_ptr<char[],std::default_delete<char[]>> safe_##d(d)
#define UNI2UTF_S(s,d) char*d=UnicodeToUtf8((s));std::unique_ptr<char[],std::default_delete<char[]>> safe_##d(d)