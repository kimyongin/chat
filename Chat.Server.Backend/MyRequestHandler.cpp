#include "MyRequestHandler.h"
#include "HttpServer.h"
#include "WebSocketMessage.h"
#include "ChatService.h"
#include "../Common/Converter.h"
#include <Poco/Net/WebSocket.h>
#include <Poco/StreamCopier.h>
#include <boost/thread/shared_lock_guard.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>
#include <ostream>
#include <locale>
#include <exception>
#include <codecvt>
#include <fstream>
#include <streambuf>
#include <future>
#include <functional>

// ------------------------------------------------------------------------------------------
// CHomeRequestHandler
// ------------------------------------------------------------------------------------------
CHomeRequestHandler::CHomeRequestHandler(Poco::Net::HTTPResponse::HTTPStatus status)
	:m_status(status)
{

}

void CHomeRequestHandler::handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response)
{
	response.setStatus(m_status);
	response.setContentType("text/html");

	std::ostream& out = response.send();
	out << "<h1>Hello! I'm Local Message Server!</h1>"
		<< "<p>Host: " << request.getHost() << "</p>"
		<< "<p>Method: " << request.getMethod() << "</p>"
		<< "<p>URI: " << request.getURI() << "</p>";
	out.flush();
}

// ------------------------------------------------------------------------------------------
// CFileRequestHandler
// ------------------------------------------------------------------------------------------
CFileRequestHandler::CFileRequestHandler(const boost::filesystem::path& filePath) :m_filePath(filePath)
{

}

BOOL CFileRequestHandler::IsExist(const std::string& uri, const std::string& rootPath, OUT boost::filesystem::path& _filePath)
{
	if (uri.empty() || uri.compare("/") == 0) {
		return FALSE;
	}

	std::string searchPath = rootPath;
	if (searchPath.empty()) {
		// 루트가 정해져있지 않다면 현재경로에서 찾는다.
		searchPath = boost::filesystem::current_path().generic_string();
	}

	std::string uri_ = uri;
	std::size_t pos = uri_.find_last_of("?");
	if (pos != std::string::npos) {
		uri_.erase(uri_.begin() + pos, uri_.end());
	}
	boost::system::error_code ec;
	boost::filesystem::path path = boost::filesystem::canonical(searchPath + uri_, ec);
	if (ec) {
		path = boost::filesystem::canonical(searchPath + uri_ + ".html", ec);
	}
	_filePath = path;
	return boost::filesystem::exists(path);
}

void CFileRequestHandler::handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response)
{
	try
	{
		const std::string EXT_HTML = ".html";
		const std::string EXT_JAVASCRIPT = ".js";
		const std::string EXT_CSS = ".css";
		const std::string EXT_PNG = ".png";
		const std::string EXT_JSON = ".json";

		response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
		std::string mediaType;
		if (m_filePath.extension().compare(EXT_HTML) == 0) {
			mediaType = "text/html";
		}
		else if (m_filePath.extension().compare(EXT_JAVASCRIPT) == 0) {
			mediaType = "application/javascript";
		}
		else if (m_filePath.extension().compare(EXT_CSS) == 0) {
			mediaType = "text/css";
		}
		else if (m_filePath.extension().compare(EXT_PNG) == 0) {
			mediaType = "image/png";
		}
		else if (m_filePath.extension().compare(EXT_JSON) == 0) {
			mediaType = "application/json";
		}
		std::string temp = m_filePath.generic_string();
		response.sendFile(Converter::ansi_to_utf8(temp), mediaType);
	}
	catch (std::exception ex)
	{
		BOOST_LOG_TRIVIAL(error) << "[CFileRequestHandler Error] " << request.getURI() << " " << m_filePath.generic_string();
	}
}

// ------------------------------------------------------------------------------------------
// CChatRequestHandler
// ------------------------------------------------------------------------------------------
void CChatRequestHandler::handleRequest(Poco::Net::HTTPServerRequest &request, Poco::Net::HTTPServerResponse &response)
{
	try
	{
		if (request.getURI().find("/chat/ws") == 0)
		{
			Poco::Net::WebSocket spWebSocket(request, response);
			CChatService::GetInstance().AddWebSocket(spWebSocket);
		}
	}
	catch (const Poco::Net::WebSocketException& exc)
	{
		// For any errors with HTTP connect, respond to the request. 
		// For any websocket error, exiting this function will destroy the socket.
		switch (exc.code())
		{
		case Poco::Net::WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
			response.set("Sec-WebSocket-Version", Poco::Net::WebSocket::WEBSOCKET_VERSION);
			// fallthrough
		case Poco::Net::WebSocket::WS_ERR_NO_HANDSHAKE:
		case Poco::Net::WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:
		case Poco::Net::WebSocket::WS_ERR_HANDSHAKE_NO_KEY:
			response.setStatusAndReason(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
			response.setContentLength(0);
			response.send();
			break;
		}
	}
}
