#pragma once
#define POCO_STATIC 
#include <vector>
#include <deque>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <condition_variable>
#include <boost/optional.hpp>
#include <Poco/Net/WebSocket.h>
#include <boost/thread/shared_mutex.hpp>
#include <boost/property_tree/ptree.hpp>
#include "ChatRepository.h"

// -----------------------------------------------------------------------
// CChatMessage
// -----------------------------------------------------------------------
class CChatMessage
{
public:
	CChatMessage(__int64 seq, std::string& sender, std::string& content, const std::string& time, const boost::property_tree::ptree& msg)
		:m_seq(seq), m_sender(sender), m_content(content), m_time(time), m_msg(msg){}
	inline __int64 GetSeq(){ return m_seq; }
	inline std::string& GetSender() { return m_sender; }
	inline std::string& GetContent() { return m_content; }
	inline std::string& GetTime() { return m_time; }
	inline boost::property_tree::ptree& GetMsg(){ return m_msg; }
private:
	__int64 m_seq;
	std::string m_sender;
	boost::property_tree::ptree m_msg;
	std::string m_content;
	std::string m_time;
};

// -----------------------------------------------------------------------
// CChatRoom
// -----------------------------------------------------------------------
class CChatRoom
{
public:
	CChatRoom(std::string roomName) :m_roomName(roomName){}

public:
	typedef std::unordered_map<std::string/*UserName*/, boost::optional<Poco::Net::WebSocket>/*User Socket*/> UserContainer;
	typedef std::unordered_map<std::string/*UserName*/, __int64/*LastReadSeq*/> ReadStateContainer;
	typedef std::unordered_map<std::string/*UserName*/, std::string/*NickName*/> RoomNickNameContainer;
	typedef std::map<__int64, std::shared_ptr<CChatMessage>, std::greater<__int64>> MessageContainer; // 거꾸로 정렬

	__int64 GetUserLastReadSeq(const std::string& userName);
	__int64 SetUserLastReadSeq(const std::string& userName, __int64 seq);

	__int64 GetUserStartReadSeq(const std::string& userName);
	__int64 SetUserStartReadSeq(const std::string& userName, __int64 seq);

	std::string GetUserRoomNickName(const std::string& userName);
	void SetUserRoomNickName(const std::string& userName, const std::string& roomNickName);

	__int64 GetFirstSeq();	// 메모리에 들고 있는 첫번째 메세지의 번호
	__int64 GetLastSeq(); // 메모리에 들고 있는 마지막 메세지의 번호

	void AddMessage(std::shared_ptr<CChatMessage> chatMsg);
	boost::optional<std::tuple<std::string, std::string, std::string>> GetLastMessage();
	__int64 GetMessages(std::string& msg, __int64 startSeq, __int64 count);

	boost::optional<Poco::Net::WebSocket> AddWebSocket(std::string userName, boost::optional<Poco::Net::WebSocket> spWebSocket);
	boost::optional<Poco::Net::WebSocket> DeleteWebSocket(std::string userName);
	boost::optional<Poco::Net::WebSocket> FindWebSocket(std::string userName);

	inline UserContainer& GetUserContainer(){ return m_userContainer; }
	inline std::string& GetRoomName(){ return m_roomName; }
private:
	std::string m_roomName;
	UserContainer m_userContainer;
	MessageContainer m_messageContainer;
	ReadStateContainer m_lastReadStateContainer;
	ReadStateContainer m_startReadStateContainer;
	RoomNickNameContainer m_roomNickNameContainer;
};
