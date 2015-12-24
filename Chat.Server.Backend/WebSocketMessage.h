#pragma once
#define POCO_STATIC 

#include <string>
#include <vector>

enum websocket_message_type
{
	WEB_SOCKET_BINARY_MESSAGE_TYPE,
	WEB_SOCKET_BINARY_FRAGMENT_TYPE,
	WEB_SOCKET_UTF8_MESSAGE_TYPE,
	WEB_SOCKET_UTF8_FRAGMENT_TYPE
};

class CWebSocketMessage
{
public:
	CWebSocketMessage(int n);
	CWebSocketMessage(unsigned char* buffer, int flags, int n);
	CWebSocketMessage(const std::string& msg);

	int get_flags() const;
	int get_size() const;
	std::vector<unsigned char>& get_data();
	const std::vector<unsigned char>& get_data() const;
	websocket_message_type get_message_type() const;

	const std::string as_string() const;

private:
	std::vector<unsigned char> m_data;
	websocket_message_type m_msg_type;
};
