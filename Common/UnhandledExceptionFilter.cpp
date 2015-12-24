#include "UnhandledExceptionFilter.h"
#include <DbgHelp.h>
#include <Shlwapi.h>
#include <string>
#pragma comment(lib, "Shlwapi.lib")

using namespace std;

typedef BOOL(WINAPI *MINIDUMPWRITEDUMP) (HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, CONST PMINIDUMP_EXCEPTION_INFORMATION, CONST PMINIDUMP_USER_STREAM_INFORMATION, CONST PMINIDUMP_CALLBACK_INFORMATION);

VOID GetHostName(OUT std::string& _hostName)
{
	CHAR szBuffer[MAX_COMPUTERNAME_LENGTH + 1]; ZeroMemory(szBuffer, sizeof(szBuffer));
	DWORD dwBufferSize = MAX_COMPUTERNAME_LENGTH + 1;
	if (GetComputerNameA(szBuffer, &dwBufferSize)){
		_hostName.assign(szBuffer);
	}else{
		_hostName = "Unknown";
	}
}

VOID GetModuleName(OUT std::string& _moduleName)
{
	CHAR szBuffer[MAX_PATH + 1]; ZeroMemory(szBuffer, sizeof(szBuffer));
	CHAR szResult[MAX_PATH + 1]; ZeroMemory(szResult, sizeof(szResult));
	INT32 nPathLength = 0, nFileNameLength = 0;
	CHAR *pFirstIndex, *pSecIndex;

	nPathLength = GetModuleFileNameA(NULL, szBuffer, sizeof(szBuffer));
	pFirstIndex = strrchr(szBuffer, '\\');
	pSecIndex = strrchr(szBuffer, '\0');
	nFileNameLength = pSecIndex - pFirstIndex;
	CopyMemory(szResult, pFirstIndex + 1, nFileNameLength * sizeof(TCHAR));
	_moduleName.assign(szResult);
}

VOID GetModulePath(OUT std::string& _modulePath)
{
	CHAR szBuffer[MAX_PATH + 1]; ZeroMemory(szBuffer, sizeof(szBuffer));
	INT32 nPathLength = 0, nFileNameLength = 0;

	nPathLength = GetModuleFileNameA(NULL, szBuffer, sizeof(szBuffer));
	::PathRemoveFileSpecA(szBuffer);
	_modulePath.assign(szBuffer);
}

BOOL CreateFolderExclusive(IN LPCSTR lpszFilePath)
{
	CHAR szPathBuffer[MAX_PATH];
	size_t len = strlen(lpszFilePath);
	for (size_t i = 0; i < len; i++)
	{
		szPathBuffer[i] = *(lpszFilePath + i);
		if (szPathBuffer[i] == ('\\') || szPathBuffer[i] == ('/'))
		{
			szPathBuffer[i + 1] = NULL;
			if (!PathFileExistsA(szPathBuffer))
			{
				if (!::CreateDirectoryA(szPathBuffer, NULL))
				{
					if (GetLastError() != ERROR_ALREADY_EXISTS)
						return FALSE;
				}
			}
		}
	}
	return TRUE;
}

HANDLE FCreateFile(IN LPCSTR lpszFilePath)
{
	if (!CreateFolderExclusive(lpszFilePath)) {
		return INVALID_HANDLE_VALUE;
	}
	return CreateFileA(
		lpszFilePath,
		GENERIC_WRITE,
		NULL,				// lock file
		NULL,
		CREATE_ALWAYS,		// 이미파일이 존재한다면 ERROR_ALREADY_EXISTS 
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
}


LONG WINAPI DefinedUnhandledExceptionFilter(__in struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	return EXCEPTION_EXECUTE_HANDLER;
}

LONG WINAPI DefinedUnhandledExceptionFilterForDump(__in struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	HMODULE dllHandle = LoadLibraryA("dbghelp.dll");
	if (dllHandle) 
	{
		MINIDUMPWRITEDUMP Dump = (MINIDUMPWRITEDUMP)GetProcAddress(dllHandle, "MiniDumpWriteDump");
		if (Dump) 
		{
			std::string modulePath, moduleName, hostName;
			GetModulePath(modulePath); GetModuleName(moduleName); GetHostName(hostName);
			CHAR lpszDumpPath[MAX_PATH + 1]; ZeroMemory(lpszDumpPath, sizeof(lpszDumpPath));
			SYSTEMTIME oT;
			GetLocalTime(&oT);
			sprintf_s(lpszDumpPath, "%s\\%s\\%s_%s_%04d%02d%02d%02d%02d%02d.dmp", 
				modulePath.c_str(), "Dump", hostName.c_str(), moduleName.c_str(),
				oT.wYear, oT.wMonth, oT.wDay, oT.wHour, oT.wMinute, oT.wSecond);
			HANDLE fileHandle = FCreateFile(lpszDumpPath);
			if (INVALID_HANDLE_VALUE != fileHandle) {
				_MINIDUMP_EXCEPTION_INFORMATION MiniDumpExceptionInfo;
				MiniDumpExceptionInfo.ThreadId = GetCurrentThreadId();
				MiniDumpExceptionInfo.ExceptionPointers = ExceptionInfo;
				MiniDumpExceptionInfo.ClientPointers = NULL;
				BOOL success = Dump(GetCurrentProcess(), GetCurrentProcessId(), fileHandle, MiniDumpNormal, &MiniDumpExceptionInfo, NULL, NULL);
			}
			CloseHandle(fileHandle);
		}
		FreeLibrary(dllHandle);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}
