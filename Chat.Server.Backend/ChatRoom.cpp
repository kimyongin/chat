#include "ChatService.h"
#include "WebSocketMessage.h"
#include "../Common/Converter.h"
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/shared_lock_guard.hpp>
#include <thread>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time.hpp>
#include "ChatRepository.h"
#include "ChatRoom.h"

// -----------------------------------------------------------------------
// CChatRoom
// -----------------------------------------------------------------------
boost::optional<Poco::Net::WebSocket> CChatRoom::AddWebSocket(std::string userName, boost::optional<Poco::Net::WebSocket> spWebSocket)
{
	boost::optional<Poco::Net::WebSocket> oldSocket;
	auto iter = m_userContainer.find(userName);
	if (iter == m_userContainer.end()){
		m_userContainer.insert(std::make_pair(userName, spWebSocket));
	}
	else{
		oldSocket = iter->second;
		iter->second = spWebSocket;
	}
	return oldSocket;
}

boost::optional<Poco::Net::WebSocket> CChatRoom::DeleteWebSocket(std::string userName)
{
	boost::optional<Poco::Net::WebSocket> delSocket;
	auto iter = m_userContainer.find(userName);
	if (iter != m_userContainer.end()){
		delSocket = iter->second;
		m_userContainer.erase(iter);
	}
	return delSocket;
}

boost::optional<Poco::Net::WebSocket> CChatRoom::FindWebSocket(std::string userName)
{
	auto iter = m_userContainer.find(userName);
	if (iter == m_userContainer.end()){
		return boost::none;
	}
	return iter->second;
}

__int64 CChatRoom::GetMessages(std::string& msg, __int64 startSeq, __int64 count)
{
	// 메세지목록(reverse)
	boost::property_tree::ptree messageDataPT;
	auto iterMsg = m_messageContainer.find(startSeq);
	if (iterMsg == m_messageContainer.end()){
		return 0;
	}

	for (; iterMsg != m_messageContainer.end() && count--; ++iterMsg){
		messageDataPT.push_back(std::make_pair("", iterMsg->second->GetMsg()));
	}

	// 유저목록
	boost::property_tree::ptree userDataPT;
	auto iterUser = m_userContainer.begin();
	for (; iterUser != m_userContainer.end(); ++iterUser){
		boost::property_tree::ptree userPT;
		userPT.put("name", iterUser->first);
		userPT.put("last_read_seq", m_lastReadStateContainer[iterUser->first]);
		userPT.put("start_read_seq", m_startReadStateContainer[iterUser->first]);
		userDataPT.push_back(std::make_pair("", userPT));
	}

	// 통합 : 메세지목록 + 유저목록
	boost::property_tree::ptree unionDataPT;
	unionDataPT.push_back(std::make_pair("messages", messageDataPT));
	unionDataPT.push_back(std::make_pair("users", userDataPT));

	boost::property_tree::ptree resultPT;
	resultPT.put("event", "message_s");
	resultPT.put_child("data", unionDataPT);

	std::ostringstream resultBuf;
	write_json(resultBuf, resultPT);
	msg = std::move(Converter::ansi_to_utf8(resultBuf.str()));
	return messageDataPT.size();
}

__int64 CChatRoom::SetUserLastReadSeq(const std::string& userName, __int64 seq)
{
	m_lastReadStateContainer[userName] = seq;
	return seq;
}

__int64 CChatRoom::GetUserLastReadSeq(const std::string& userName)
{
	auto iter = m_lastReadStateContainer.find(userName);
	if (iter == m_lastReadStateContainer.end()){
		return 0;
	}
	return iter->second;
}

__int64 CChatRoom::SetUserStartReadSeq(const std::string& userName, __int64 seq)
{
	m_startReadStateContainer[userName] = seq;
	return seq;
}

__int64 CChatRoom::GetUserStartReadSeq(const std::string& userName)
{
	auto iter = m_startReadStateContainer.find(userName);
	if (iter == m_startReadStateContainer.end()){
		return 0;
	}
	return iter->second;
}

std::string CChatRoom::GetUserRoomNickName(const std::string& userName)
{
	auto iter = m_roomNickNameContainer.find(userName);
	if (iter == m_roomNickNameContainer.end()){
		return "";
	}
	return iter->second;
}

void CChatRoom::SetUserRoomNickName(const std::string& userName, const std::string& roomNickName)
{
	m_roomNickNameContainer[userName] = roomNickName;
}

void CChatRoom::AddMessage(std::shared_ptr<CChatMessage> chatMsg)
{
	m_messageContainer.insert(std::make_pair(chatMsg->GetSeq(), chatMsg));
}

boost::optional<std::tuple<std::string, std::string, std::string>> CChatRoom::GetLastMessage()
{
	if (m_messageContainer.empty()){
		return boost::none;
	}
	std::shared_ptr<CChatMessage> spMsg = m_messageContainer.begin()->second;
	return std::make_tuple(spMsg->GetSender(), spMsg->GetContent(), spMsg->GetTime());
}

__int64 CChatRoom::GetFirstSeq()
{
	if (m_messageContainer.empty())
		return 0;

	return m_messageContainer.rbegin()->second->GetSeq();
}

__int64 CChatRoom::GetLastSeq()
{
	if (m_messageContainer.empty())
		return 0;

	return m_messageContainer.begin()->second->GetSeq();
}
