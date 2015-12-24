#include "ChatService.h"
#include "ChatRepository.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/log/trivial.hpp>
#include <Shlwapi.h>

using namespace sqlite;

bool DirExists(LPCSTR path)
{
	DWORD ftyp = GetFileAttributesA(path);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

std::string CChatRepository::GetDBPath()
{
	auto list = { "D:\\YSR2000\\Database", "C:\\YSR2000\\Database", "E:\\YSR2000\\Database", "F:\\YSR2000\\Database" };
	std::string findPath = "";
	for each(auto path in list){
		if (DirExists(path)) {
			findPath.assign(path);
			findPath += "\\livetalk.db";
			break;
		}
	}
	if (findPath.empty()){
		findPath.assign("livetalk.db");
	}
	return findPath;
}

CChatRepository::CChatRepository(std::string dbName)
	:m_db(dbName), m_dbName(dbName)
{
	
}

CChatRepository::~CChatRepository()
{

}

VOID CChatRepository::Init()
{
	// https://www.sqlite.org/datatype3.html

	//m_db <<"DROP TABLE IF EXISTS CHAT_ROOM;";
	m_db <<
		"CREATE TABLE IF NOT EXISTS CHAT_ROOM ("
		"	ROOM_NAME CHAR(36) NOT NULL,"		// 방 이름(uuid)
		"	ROOM_NICK_NAME VARCHAR(255) NULL,"	// 유저가 설정한 방 별칭
		"	USER_NAME VARCHAR(255) NOT NULL,"	// 유저 이름
		"   START_READ_SEQ BIGINT NOT NULL,"	// 방에 들어왔을때의 메세지 번호
		"   LAST_READ_SEQ BIGINT NOT NULL,"		// 마지막으로 읽은 메세지 번호
		"	UNIQUE(ROOM_NAME, USER_NAME)"
		");";

	//m_db <<"DROP TABLE IF EXISTS CHAT_MESSAGE;";
	m_db <<
		"CREATE TABLE IF NOT EXISTS CHAT_MESSAGE ("
		"	ROOM_NAME CHAR(36) NOT NULL,"		// 방 이름(uuid)
		"   USER_NAME VARCHAR(255) NOT NULL,"	// 유저 이름
		"   MESSAGE TEXT NOT NULL,"				// 메세지 내용
		"   MESSAGE_SEQ BIGINT NOT NULL,"		// 메세지 번호
		"   SEND_TIME DATETIME NOT NULL"		// 보낸시간
		");";

	m_db << "CREATE INDEX IF NOT EXISTS INDEX_ROOM_MESSAGE ON CHAT_MESSAGE("
		"	ROOM_NAME,"
		"	MESSAGE_SEQ"
		"	);";

	m_db << "CREATE INDEX IF NOT EXISTS INDEX_ROOM ON CHAT_ROOM("
		"	ROOM_NAME"
		"	);";
}

std::string CChatRepository::CreateRoomName()
{
	// UUID in ASCII have 32 characters plus 4 separators, 36
	// ex : "7feb24af-fc38-44de-bc38-04defc3804de"
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	return boost::lexical_cast<std::string>(uuid);
}

boost::optional<std::string> CChatRepository::SelectRoom(const std::string& userName, const boost::property_tree::ptree& inviteList)
{
	std::string result;

	try
	{
		std::stringstream ss;

		ss << "SELECT A.ROOM_NAME, COUNT(1) AS TCNT FROM CHAT_ROOM A";
		ss << "	JOIN(SELECT ROOM_NAME, COUNT(1) AS SCNT FROM CHAT_ROOM WHERE USER_NAME IN(";
		ss << "'" << userName << "'";
		for each(auto user in inviteList){
			boost::optional<std::string> friendName = user.second.get_optional<std::string>("name");
			if (friendName){
				ss << ", '" << friendName.get() << "'";
			}
		}
		ss << "	) GROUP BY ROOM_NAME HAVING COUNT(1) = " << inviteList.size() + 1 << ") B";
		ss << "	ON B.ROOM_NAME = A.ROOM_NAME";
		ss << "	GROUP BY A.ROOM_NAME";
		ss << "	HAVING TCNT = " << inviteList.size() + 1;
		
		std::string query = ss.str();
		m_db << query >> result;
	}
	catch (std::exception ex){
		return boost::none;
	}

	if (result.empty()){
		return boost::none;
	}

	return boost::optional<std::string>(result);

}

INT CChatRepository::SelectRoomUser(std::string roomName, std::vector<UserInfo>& vecUsers)
{
	m_db 
		<< "SELECT USER_NAME, ROOM_NICK_NAME, START_READ_SEQ, LAST_READ_SEQ FROM CHAT_ROOM WHERE ROOM_NAME = ?;" 
		<< roomName
		>> [&](std::string userName, std::string roomNickName, __int64 startReadSeq, __int64 lastReadSeq) {
		vecUsers.push_back(std::make_tuple(userName, roomNickName, startReadSeq, lastReadSeq));
	};
	return vecUsers.size();
}

BOOL CChatRepository::InsertRoom(const std::string& roomName, const std::string& userName, const __int64 seq)
{
	try
	{
		m_db << "INSERT OR IGNORE INTO CHAT_ROOM (ROOM_NAME, USER_NAME, START_READ_SEQ, LAST_READ_SEQ) VALUES (?,?,?,?);"
			<< roomName
			<< userName
			<< seq
			<< seq;
	}
	catch (std::exception ex){
		return false;
	}
	return true;
}

BOOL CChatRepository::UpdateRoom(const std::string& roomName, const std::string& userName, const __int64 lastReadSeq)
{
	try
	{
		std::stringstream ss;
		ss	<< "UPDATE CHAT_ROOM SET LAST_READ_SEQ = " << lastReadSeq
			<< "	WHERE ROOM_NAME = '" << roomName << "' AND USER_NAME = '" << userName << "'";
		m_db << ss.str();
	}
	catch (std::exception ex){
		return false;
	}
	return true;
}


BOOL CChatRepository::UpdateRoom(const std::string& roomName, const std::string& userName, const std::string& roomNickName)
{
	try
	{
		std::stringstream ss;
		ss << "UPDATE CHAT_ROOM SET ROOM_NICK_NAME = '" << roomNickName << "'"
			<< "	WHERE ROOM_NAME = '" << roomName << "' AND USER_NAME = '" << userName << "'";
		m_db << ss.str();
	}
	catch (std::exception ex){
		return false;
	}
	return true;
}


BOOL CChatRepository::DeleteRoom(const std::string& roomName, const std::string& userName)
{
	try
	{
		std::stringstream ss;
		ss << "DELETE FROM CHAT_ROOM WHERE ROOM_NAME = '" << roomName << "' AND USER_NAME = '" << userName << "'";
		std::string query = ss.str();
		m_db << query;
	}
	catch (std::exception ex){
		return false;
	}
	return true;
}

BOOL CChatRepository::SelectMessage(CChatRoom* room, const std::string& roomName, __int64 startSeq, __int64 count)
{
	std::stringstream ss;
	ss	<< "SELECT USER_NAME, MESSAGE, MESSAGE_SEQ, SEND_TIME FROM CHAT_MESSAGE WHERE ROOM_NAME = '" << roomName << "'";
	if (startSeq > 0){
		ss << "	AND MESSAGE_SEQ < " << startSeq << " AND MESSAGE_SEQ > " << startSeq - count;
	}
	ss << "	ORDER BY MESSAGE_SEQ ASC";
	std::string query = ss.str();

	boost::property_tree::ptree messageDataPT;
	try{
		m_db
			<< query
			>> [&](std::string userName, std::string message, __int64 messageSeq, std::string sendTime) {

			messageDataPT.clear();
			messageDataPT.put("result", "success");
			messageDataPT.put("user_name", userName);
			messageDataPT.put("type", "message");
			messageDataPT.put("seq", messageSeq);
			messageDataPT.put("content", message);
			messageDataPT.put("time", sendTime);

			room->AddMessage(std::make_shared<CChatMessage>(messageSeq, userName, message, sendTime, messageDataPT));
		};
	}catch (std::exception ex){
		return FALSE;
	}
	return TRUE;
}

INT CChatRepository::SelectLastMessagePerRoom(const std::string userName, std::vector<MessageInfo>& vecMessages)
{
	try{
		m_db <<
			"SELECT R.ROOM_NAME, M.USER_NAME, M.MESSAGE, M.MESSAGE_SEQ, M.SEND_TIME"
			"	FROM CHAT_ROOM AS R INNER JOIN CHAT_MESSAGE AS M ON M.ROOM_NAME = R.ROOM_NAME"
			"	WHERE M.MESSAGE_SEQ = (SELECT MAX(MESSAGE_SEQ) FROM CHAT_MESSAGE WHERE ROOM_NAME = R.ROOM_NAME)"
			"	AND R.USER_NAME = ?"
			<< userName
			>> [&](std::string roomName, std::string userName, std::string message, __int64 messageSeq, std::string sendTime) {

			vecMessages.push_back(std::make_tuple(roomName, userName, message, messageSeq, sendTime));
		};
	}
	catch (std::exception ex){
		return -1;
	}
	return vecMessages.size();
}

BOOL CChatRepository::InsertMessage(const std::string& roomName, const std::string& userName, const std::string& message, const __int64 messageSeq, const std::string& sendTime)
{
	try
	{
		//"yyyy-MM-dd HH:mm:ss"
		m_db << "INSERT INTO CHAT_MESSAGE(ROOM_NAME, USER_NAME, MESSAGE, MESSAGE_SEQ, SEND_TIME) VALUES (?,?,?,?,?);"
			<< roomName
			<< userName
			<< message
			<< messageSeq
			<< sendTime;
	}
	catch (std::exception ex){
		return FALSE;
	}
	return TRUE;
}