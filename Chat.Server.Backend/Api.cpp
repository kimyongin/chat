// YSarang.Communicate.cpp : DLL ���� ���α׷��� ���� ������ �Լ��� �����մϴ�.
//

#include "../Common/Logger.h"
#include "HttpServer.h"
#include "MyRequestHandler.h"
#include <boost/log/trivial.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <ostream>
#include <sstream>
#include "Api.h"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")

// ----------------------------------------------------------------------
// Init & Release
// ----------------------------------------------------------------------
VOID TInit()
{
	Common::CLogger::Init();
}

VOID TRelease()
{
	Common::CLogger::Release();
}

// ----------------------------------------------------------------------
// Log
// ----------------------------------------------------------------------
DWORD AddLogConsume(fpLogConsume fp)
{
	Common::CLogger::add_stream_sink(fp);
	return NO_ERROR;
}

DWORD SetLogLevel(LogLevel level)
{
	BOOST_LOG_TRIVIAL(info) << "SetLogLevel(" << level << ")";
	Common::CLogger::SetLevel(level);
	return NO_ERROR;
}

LogLevel GetLogLevel()
{
	return (LogLevel)Common::CLogger::GetLevel();
}

// ----------------------------------------------------------------------
// HTTP Server
// ----------------------------------------------------------------------
DWORD HTTPServerCreate(IN LPCSTR port, IN LPCSTR rootPath)
{
	BOOST_LOG_TRIVIAL(info) << "HttpServerCreate";

	int openPort = 0;
	try {
		openPort = boost::lexical_cast<int>(port);
	}
	catch (boost::bad_lexical_cast &) {
		BOOST_LOG_TRIVIAL(error) << "�߸��� ��Ʈ��ȣ �Դϴ�.";
		return ERROR_INVALID_PARAMETER;
	}

	CHttpServer* pObj = new CHttpServer(openPort, rootPath);
	return (DWORD)pObj;
}

VOID HTTPServerDestroy(IN DWORD pServerObj)
{
	BOOST_LOG_TRIVIAL(info) << "HttpServerDestroy";
	if (pServerObj == NULL) {
		BOOST_LOG_TRIVIAL(error) << "�ν��Ͻ��� ��ȿ���� �ʽ��ϴ�.";
		return;
	}
	delete (CHttpServer*)pServerObj;
}

DWORD HTTPServerStart(IN DWORD pServerObj)
{
	BOOST_LOG_TRIVIAL(info) << "HttpServerStart";
	if (pServerObj == NULL) {
		BOOST_LOG_TRIVIAL(error) << "�ν��Ͻ��� ��ȿ���� �ʽ��ϴ�.";
		return ERROR_INVALID_PARAMETER;
	}

	DWORD ec;
	ec = ((CHttpServer*)pServerObj)->Start();
	if (ec != NO_ERROR) {
		return ec;
	}

	return ec;
}

DWORD HTTPServerSessionList(IN DWORD pServerObj, OUT std::string& list)
{
	BOOST_LOG_TRIVIAL(info) << "HttpServerSessionList";
	return NO_ERROR;
}
