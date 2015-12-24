#pragma once
#define POCO_STATIC 
#include <memory>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Net/NetException.h>
#include <Poco/Timespan.h>
#include "WebSocketMessage.h"

class CHttpServer
{
public:
	CHttpServer(int openPort, std::string rootPath);
	~CHttpServer();
	DWORD Start();
	const std::string& GetRootPath();
private:
	Poco::Net::ServerSocket m_server_socket;
	Poco::Net::HTTPServer m_http_server;
	const std::string m_rootPath;
};

class CRequestHandlerFactory : public Poco::Net::HTTPRequestHandlerFactory
{
public:
	CRequestHandlerFactory(CHttpServer& refHttpServer);
	Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest& request);

private:
	CHttpServer& m_refServer;
};
