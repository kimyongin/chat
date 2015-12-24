#include "HttpServer.h"
#include "MyRequestHandler.h"
#include "ChatService.h"
#include <fstream>
#include <streambuf>
#include <regex>
#include <boost/log/trivial.hpp>

CHttpServer::CHttpServer(int openPort, std::string rootPath)
	: m_server_socket(openPort)
	, m_http_server(new CRequestHandlerFactory(std::ref(*this)), m_server_socket, new Poco::Net::HTTPServerParams())
	, m_rootPath(rootPath)
{
}

CHttpServer::~CHttpServer()
{
	m_http_server.stopAll();
	m_server_socket.close();
}

DWORD CHttpServer::Start()
{
	m_http_server.start();
	CChatService::GetInstance().RunService();
	return NO_ERROR;
}

const std::string& CHttpServer::GetRootPath()
{
	return m_rootPath;
}

CRequestHandlerFactory::CRequestHandlerFactory(CHttpServer& refHttpServer) 
	: m_refServer(refHttpServer)
{

}

Poco::Net::HTTPRequestHandler* CRequestHandlerFactory::createRequestHandler(const Poco::Net::HTTPServerRequest& request)
{
	BOOST_LOG_TRIVIAL(debug) << request.getURI();
	boost::filesystem::path filePath;
	if (request.getURI().compare("/chat/ws") == 0)
	{
		return new CChatRequestHandler();
	}
	else if (CFileRequestHandler::IsExist(request.getURI(), m_refServer.GetRootPath(), filePath))
	{
		return new CFileRequestHandler(filePath);
	}
	else if (request.getURI().empty() || request.getURI().compare("/") == 0){
		return new CHomeRequestHandler(Poco::Net::HTTPResponse::HTTPStatus::HTTP_OK);
	}

	BOOST_LOG_TRIVIAL(error) << "HTTP NOT FOUND (" << request.getURI() << ")";
	return new CHomeRequestHandler(Poco::Net::HTTPResponse::HTTPStatus::HTTP_NOT_FOUND);
}