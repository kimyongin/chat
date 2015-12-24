#pragma once
#define POCO_STATIC 

#include <memory>
#include <vector>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Net/HTTPResponse.h>
#include <boost/thread/shared_mutex.hpp>
#include <boost/filesystem.hpp>
#include <future>

// ------------------------------------------------------------------------------------------
// CHomeRequestHandler
// ------------------------------------------------------------------------------------------
class CHomeRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
	CHomeRequestHandler(Poco::Net::HTTPResponse::HTTPStatus status);
	virtual void handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response);
private:
	Poco::Net::HTTPResponse::HTTPStatus m_status;
};

// ------------------------------------------------------------------------------------------
// CFileRequestHandler
// ------------------------------------------------------------------------------------------
class CFileRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:
	CFileRequestHandler(const boost::filesystem::path& filePath);
	static BOOL IsExist(const std::string& uri, const std::string& rootPath, OUT boost::filesystem::path& _filePath);
	virtual void handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response);
private:
	const boost::filesystem::path m_filePath;
};

// ------------------------------------------------------------------------------------------
// CChatRequestHandler
// ------------------------------------------------------------------------------------------
class CChatRequestHandler : public Poco::Net::HTTPRequestHandler
{
public:	
	virtual void handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response);
};
