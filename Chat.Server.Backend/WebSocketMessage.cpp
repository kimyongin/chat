#include "WebSocketMessage.h"
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Net/NetException.h>

CWebSocketMessage::CWebSocketMessage(int n)
{
	m_data.resize(n);
}

CWebSocketMessage::CWebSocketMessage(unsigned char* buffer, int flags, int n) : m_data(buffer, buffer + n)
{
	switch (flags)
	{
	case Poco::Net::WebSocket::FRAME_TEXT:
		m_msg_type = websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE;
		break;
	case Poco::Net::WebSocket::FRAME_BINARY:
		m_msg_type = websocket_message_type::WEB_SOCKET_BINARY_MESSAGE_TYPE;
		break;
	}
}

CWebSocketMessage::CWebSocketMessage(const std::string& msg) : m_data(msg.begin(), msg.end())
{
	m_msg_type = websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE;
}

int CWebSocketMessage::get_size() const
{
	return m_data.size();
}

std::vector<unsigned char>& CWebSocketMessage::get_data()
{
	return m_data;
}

const std::vector<unsigned char>& CWebSocketMessage::get_data() const
{
	return m_data;
}

websocket_message_type CWebSocketMessage::get_message_type()const
{
	return m_msg_type;
}

int CWebSocketMessage::get_flags() const
{
	int flags = 0;

	switch (m_msg_type)
	{
	case websocket_message_type::WEB_SOCKET_UTF8_FRAGMENT_TYPE:
		flags = Poco::Net::WebSocket::FRAME_OP_CONT | Poco::Net::WebSocket::FRAME_OP_TEXT;
		break;
	case websocket_message_type::WEB_SOCKET_UTF8_MESSAGE_TYPE:
		flags = Poco::Net::WebSocket::FRAME_FLAG_FIN | Poco::Net::WebSocket::FRAME_OP_TEXT;
		break;
	case websocket_message_type::WEB_SOCKET_BINARY_FRAGMENT_TYPE:
		flags = Poco::Net::WebSocket::FRAME_OP_CONT | Poco::Net::WebSocket::FRAME_OP_BINARY;
		break;
	case websocket_message_type::WEB_SOCKET_BINARY_MESSAGE_TYPE:
		flags = Poco::Net::WebSocket::FRAME_FLAG_FIN | Poco::Net::WebSocket::FRAME_OP_BINARY;
		break;
	}

	return flags;
}

const std::string CWebSocketMessage::as_string() const
{
	std::string temp(m_data.begin(), m_data.end());
	return temp;
}