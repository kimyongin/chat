#pragma once

#ifdef API_EXPORTS
#define API extern "C" __declspec(dllexport)
#else
#define API extern "C" __declspec(dllimport)
#endif

#include <Windows.h>
#include <functional>
#include <string>

typedef std::function<void(const std::string& message)> fpLogConsume;
typedef std::function<void(const std::string& data)> fpContainerUpdate;

typedef enum
{
	trace = 0,
	debug = 1,
	info = 2,
	warning = 3,
	error = 4,
	fatal = 5
}LogLevel;

API VOID TInit();
API VOID TRelease();

API DWORD AddLogConsume(fpLogConsume fp);
API DWORD SetLogLevel(LogLevel level);
API LogLevel GetLogLevel();

API DWORD HTTPServerCreate(IN LPCSTR port, IN LPCSTR rootPath);
API VOID  HTTPServerDestroy(IN DWORD pServerObj);
API DWORD HTTPServerStart(IN DWORD pServerObj);
API DWORD HTTPServerSessionList(IN DWORD pServerObj, OUT std::string& list);