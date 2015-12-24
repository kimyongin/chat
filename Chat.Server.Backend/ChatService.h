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
#include "ChatRoom.h"

// -----------------------------------------------------------------------
// CChatWork
// -----------------------------------------------------------------------
class CChatWork
{
public:
	CChatWork(Poco::Net::WebSocket webSock, std::string& workMessage) 
		:m_webSock(webSock), m_workMessage(workMessage){}
	inline Poco::Net::WebSocket& GetWebSocket(){ return m_webSock; }
	inline std::string& GetWorkMessage(){ return m_workMessage; }
private:
	Poco::Net::WebSocket m_webSock;
	std::string m_workMessage;
};

// -----------------------------------------------------------------------
// CChatService
// -----------------------------------------------------------------------
class CChatService
{
public:
	static CChatService& GetInstance();
	void RunService();
	void RunServiceOld();
	void HandleMessage(std::shared_ptr<CChatWork> spChatWork);
	void AddWebSocket(Poco::Net::WebSocket sock);
private:
	void RunService_Calc(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);
	void RunService_Regist(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);	
	void RunService_UnRegist(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);
	void RunService_Make(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);
	void RunService_Invite(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);
	void RunService_Join(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);
	void RunService_Leave(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);
	void RunService_Users(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);
	void RunService_Message(const boost::property_tree::ptree& pt, boost::optional<Poco::Net::WebSocket> sock);
	void RunService_More(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);
	void RunService_Seq(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);
	void RunService_Close(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);
	void RunService_RoomName(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock);
private:	
	void RunService_Users();
	void RunService_UserListMake(const std::string roomName, std::string& usersMessage);
	void RunService_RoomListMake(const std::string& userName, std::string& roomsMessage);
	bool RunService_AlarmMake(const std::string& roomName, std::string& alarmMessage);
	void RunService_AlarmSend(const std::string& roomName, const std::string& userName, const std::string& alarmMessage);
	void RunService_Room(const std::string& roomName, const std::string& type);	
	void RunService_Make(const std::string& roomName, const std::string& userName);
private:	
	CChatService();	
private:
	typedef std::unordered_map<std::string/*User Name*/, Poco::Net::WebSocket/*User Socket*/> UserContainer;
	typedef std::unordered_map<std::string/*Room Name*/, std::shared_ptr<CChatRoom>> RoomContainer;
	RoomContainer m_roomContainer;

	Poco::Net::WebSocket::SocketList m_sockContainer;
	boost::shared_mutex m_sockMutex;
	std::shared_ptr<CChatRepository> m_spChatRepository;
	static CChatService ChatUserService;
};

