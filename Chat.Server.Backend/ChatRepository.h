#pragma once
#include "../Sqlite3/sqlite_modern_cpp.h"
#include <Windows.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

class CChatRoom;
class CChatRepository
{
public:
	CChatRepository(const std::string dbName);
	~CChatRepository();

public:
	VOID Init();
	std::string CreateRoomName(); // return guid
	static std::string GetDBPath();
	
	// Room
	typedef std::tuple<std::string, std::string, __int64, __int64> UserInfo;
	INT SelectRoomUser(std::string roomName, std::vector<UserInfo>& vecUsers);
	boost::optional<std::string> SelectRoom(const std::string& userName, const boost::property_tree::ptree& inviteList);
	BOOL InsertRoom(const std::string& roomName, const std::string& userName, const __int64 lastReadSeq);
	BOOL UpdateRoom(const std::string& roomName, const std::string& userName, const __int64 lastReadSeq);
	BOOL UpdateRoom(const std::string& roomName, const std::string& userName, const std::string& roomNickName);
	BOOL DeleteRoom(const std::string& roomName, const std::string& userName);
	
	// Message
	typedef std::tuple<std::string, std::string, std::string, __int64, std::string> MessageInfo;
	BOOL SelectMessage(CChatRoom* container, const std::string& roomName, __int64 startSeq, __int64 count);
	INT SelectLastMessagePerRoom(const std::string userName, std::vector<MessageInfo>& vecMessages);
	BOOL InsertMessage(const std::string& roomName, const std::string& userName, const std::string& message, const __int64 messageSeq, const std::string& sendTime);

private:
	std::string m_dbName;
	sqlite::database m_db;
};

