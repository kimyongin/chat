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
#include <boost/log/trivial.hpp>
#include "ChatRepository.h"

#define MAX_CLIENTS 256
#define LOAD_MESSAGE_COUNT 5
#define MORE_MESSAGE_COUNT 5

// -----------------------------------------------------------------------
// CChatService
// -----------------------------------------------------------------------
CChatService CChatService::ChatUserService;
CChatService& CChatService::GetInstance()
{
	return CChatService::ChatUserService;
}
CChatService::CChatService()
{
	m_roomContainer.insert(std::make_pair("all", std::make_shared<CChatRoom>("all")));
}
void CChatService::AddWebSocket(Poco::Net::WebSocket sock)
{
	BOOST_LOG_TRIVIAL(trace) << "[ChatService Connect] " << sock.address().toString();
	boost::lock_guard<boost::shared_mutex> lock(m_sockMutex); // lock_guard (컨테이너 잠금)
	if (m_sockContainer.size() > MAX_CLIENTS) {
		sock.close();
		BOOST_LOG_TRIVIAL(error) << "[ChatService Error] Max Clients";
		return;
	}
	m_sockContainer.push_back(sock);
}

void CChatService::RunService()
{
	m_spChatRepository = std::shared_ptr<CChatRepository>(new CChatRepository(CChatRepository::GetDBPath()));
	m_spChatRepository->Init();

	std::thread worker([&]()
	{
		char buffer[10240];
		int flags, bytes_received;
		Poco::Timespan timeout(0, 50000); // 마이크로세컨즈(0.05초)
		Poco::Net::WebSocket::SocketList selectList, writeList, exceptList;
		while (true)
		{			
			// --------------------------------------------------------------------------------------
			// 접속한 모든 웹소켓을 읽기목록에 복사
			// --------------------------------------------------------------------------------------
			{
				// shared_lock_guard (컨테이너 잠금)
				boost::lock_guard<boost::shared_mutex> lock(m_sockMutex);
				selectList = m_sockContainer;
			}

			// --------------------------------------------------------------------------------------
			// 읽기목록을 SELECT
			// 읽기가능한 소켓이 발생할때까지 블록되어 있는다. (타임아웃 될때까지)
			// --------------------------------------------------------------------------------------
			Poco::Net::WebSocket::select(selectList, writeList, exceptList, timeout);
			if (selectList.size() == 0) {
				Sleep(1000);
			}

			// --------------------------------------------------------------------------------------
			// 읽기가능한 소켓들을 순회
			// --------------------------------------------------------------------------------------
			for each (Poco::Net::WebSocket sock in selectList)
			{
				// --------------------------------------------------------------------------------------
				// 메세지 수신
				// --------------------------------------------------------------------------------------
				ZeroMemory(buffer, sizeof(buffer));
				try{
					bytes_received = 0;
					bytes_received = sock.receiveFrame(buffer, sizeof(buffer), flags);
				}catch (std::exception e){}

				// --------------------------------------------------------------------------------------
				// 정상 메세지라면.. 메세지 처리
				// --------------------------------------------------------------------------------------
				if (flags == Poco::Net::WebSocket::FRAME_TEXT)
				{
					std::shared_ptr<CChatWork> spChatWork = std::make_shared<CChatWork>(sock, std::string((char*)buffer));
					HandleMessage(spChatWork);
				}
				// --------------------------------------------------------------------------------------
				// 소켓이 닫혔다면.. 소켓 컨테이너에서 해당 소켓을 제거
				// --------------------------------------------------------------------------------------
				else if (bytes_received == 0 || (flags & Poco::Net::WebSocket::FRAME_OP_CLOSE) == Poco::Net::WebSocket::FRAME_OP_CLOSE)
				{
					std::shared_ptr<CChatWork> spChatWork = std::make_shared<CChatWork>(sock, std::string("{\"event\":\"close\"}"));
					HandleMessage(spChatWork);
					sock.close();

					{
						// lock_guard (컨테이너 잠금)
						boost::lock_guard<boost::shared_mutex> lock(m_sockMutex);
						Poco::Net::WebSocket::SocketList::iterator iter;
						for (iter = m_sockContainer.begin(); iter != m_sockContainer.end();)
						{
							if (iter->impl()->sockfd() == sock.impl()->sockfd())
							{
								iter = m_sockContainer.erase(iter);
							}
							else
							{
								iter++;
							}
						}
					}
				}
			}
		}
	});
	worker.detach();
}

void CChatService::HandleMessage(std::shared_ptr<CChatWork> spChatWork)
{
	if (spChatWork->GetWorkMessage().compare("thanks") == 0)
	{
		try{
			spChatWork->GetWebSocket().sendFrame("welcome", strlen("welcome"));
		}catch (std::exception e){}
		return;
	}

	boost::property_tree::ptree pt;
	try
	{		
		pt.clear();
		std::istringstream buf(spChatWork->GetWorkMessage());
		boost::property_tree::json_parser::read_json(buf, pt);
	}
	catch (boost::property_tree::json_parser::json_parser_error &je)
	{
		BOOST_LOG_TRIVIAL(error) << "[ChatService Error] Parsing Error";
		return;
	}

	boost::optional<std::string> evt = pt.get_optional<std::string>("event");
	if (!evt)
	{
		BOOST_LOG_TRIVIAL(error) << "[ChatService Error] Not Event Message";
		return;
	}

	try
	{
		if (evt.get().compare("calc") == 0)
		{
			// 더하기(테스트용)
			RunService_Calc(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("regist") == 0)
		{
			// 로그인
			RunService_Regist(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("unregist") == 0)
		{
			// 로그아웃
			RunService_UnRegist(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("make") == 0)
		{
			// 메인화면에서 친구선택해서 초대버튼 누름	
			RunService_Make(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("join") == 0)
		{
			// 채팅방에 입장
			RunService_Join(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("leave") == 0)
		{
			// 채팅방을 퇴장
			RunService_Leave(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("invite") == 0)
		{
			// 채팅방에 초대
			RunService_Invite(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("users") == 0)
		{
			// 메인화면에서 친구선택해서 초대버튼 누름	
			RunService_Users(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("message") == 0)
		{
			// 채팅방에 메세지 전송
			RunService_Message(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("more") == 0)
		{
			// 과거 메세지 요청
			RunService_More(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("seq") == 0)
		{
			// 채팅방에 메세지 전송
			RunService_Seq(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("close") == 0)
		{
			// 접속이 해제된 클라이언트가 있다.
			RunService_Close(pt, spChatWork->GetWebSocket());
		}
		else if (evt.get().compare("roomname") == 0)
		{
			// 채팅방이름 변경
			RunService_RoomName(pt, spChatWork->GetWebSocket());
		}
	}
	catch (std::exception ex)
	{
		BOOST_LOG_TRIVIAL(error) << "[ChatService Error] " << evt.get();
		return;
	}
}

void CChatService::RunService_Calc(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	// 테스트 용도 입니다.
	BOOST_LOG_TRIVIAL(trace) << "[RunService_Calc]";
	boost::property_tree::ptree responsePT;
	responsePT.put("event", "calc");
	boost::optional<std::string> val1 = pt.get_optional<std::string>("data.val1");
	boost::optional<std::string> val2 = pt.get_optional<std::string>("data.val2");
	boost::optional<std::string> op = pt.get_optional<std::string>("data.op");
	if (val1 && val2 && op)
	{
		int result = 0;
		int nVal1 = boost::lexical_cast<int>(val1.get());
		int nVal2 = boost::lexical_cast<int>(val2.get());

		if (op.get().compare("+") == 0){
			result = nVal1 + nVal2;
		}
		else if (op.get().compare("-") == 0){
			result = nVal1 - nVal2;
		}
		else if (op.get().compare("*") == 0){
			result = nVal1 * nVal2;
		}
		else if (op.get().compare("/") == 0){
			result = nVal1 / nVal2;
		}
		boost::property_tree::ptree dataPT;
		dataPT.put("result", boost::lexical_cast<std::string>(result));
		responsePT.put_child("data", dataPT);
	}else{
		boost::property_tree::ptree dataPT;
		dataPT.put("result", "invalide parameter");
		responsePT.put_child("data", dataPT);
	}

	std::ostringstream buf;
	write_json(buf, responsePT);
	std::string responseUtf8 = Converter::ansi_to_utf8(buf.str());
	try{
		sock.sendFrame(&responseUtf8[0], static_cast<int>(responseUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
	}catch (std::exception e){}
}

void CChatService::RunService_Regist(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	// 전체 채팅방(all)에 저장한다.
	boost::property_tree::ptree responsePT;
	responsePT.put("event", "regist");
	boost::optional<std::string> userName = pt.get_optional<std::string>("data.user_name");
	if (userName)
	{
		BOOST_LOG_TRIVIAL(info) << "[RunService_Regist] " << userName.get();
		auto room = m_roomContainer.find("all"); // 반드시 존재한다.
		boost::optional<Poco::Net::WebSocket> oldSock = room->second->AddWebSocket(userName.get(), sock);
		if (oldSock){
			// 기존 연결이 있는 경우 접속을 해제시킨다.
			boost::property_tree::ptree tempPT;
			tempPT.put("event", "regist");
			boost::property_tree::ptree dataPT;
			dataPT.put("result", "fail");
			tempPT.put_child("data", dataPT);
			std::ostringstream buf;
			write_json(buf, tempPT);
			std::string tempUtf8 = Converter::ansi_to_utf8(buf.str());
			try{
				oldSock->sendFrame(&tempUtf8[0], static_cast<int>(tempUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
			}catch (std::exception e){}
		}
		// 채팅방에 연결되어 있는 기존소켓들을 닫는다.
		for each(auto room in m_roomContainer){
			if (room.first.compare("all") != 0){
				auto oldRoomSock = room.second->FindWebSocket(userName.get());
				if (oldRoomSock){
					oldRoomSock->close();
				}
			}
		}

		// 유저들에게 접속을 알린다.
		RunService_Users();
		// 과거 대화방 목록을 전송한다.
		std::string roomsMessage;
		RunService_RoomListMake(userName.get(), roomsMessage);
		try{
			sock.sendFrame(&roomsMessage[0], static_cast<int>(roomsMessage.size()), Poco::Net::WebSocket::FRAME_TEXT);
		}catch (std::exception e){}

		boost::property_tree::ptree dataPT;
		dataPT.put("result", "success");
		responsePT.put_child("data", dataPT);
	}
	else
	{
		boost::property_tree::ptree dataPT;
		dataPT.put("result", "fail");
		responsePT.put_child("data", dataPT);
	}

	// 처리 결과를 알려준다.
	std::ostringstream buf;
	write_json(buf, responsePT);
	std::string responseUtf8 = Converter::ansi_to_utf8(buf.str());
	try{
		sock.sendFrame(&responseUtf8[0], static_cast<int>(responseUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
	}catch (std::exception e){}
}

void CChatService::RunService_UnRegist(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	// 전체 채팅방(all)에 저장한다.
	boost::property_tree::ptree responsePT;
	responsePT.put("event", "unregist");
	boost::optional<std::string> userName = pt.get_optional<std::string>("data.user_name");
	if (userName)
	{
		auto room = m_roomContainer.find("all"); // 반드시 존재한다.
		room->second->DeleteWebSocket(userName.get());

		boost::property_tree::ptree dataPT;
		dataPT.put("result", "success");
		responsePT.put_child("data", dataPT);

		// 유저들에게 접속을 알린다.
		RunService_Users();
	}
	else
	{
		boost::property_tree::ptree dataPT;
		dataPT.put("result", "fail");
		responsePT.put_child("data", dataPT);
	}

	// 처리 결과를 알려준다.
	std::ostringstream buf;
	write_json(buf, responsePT);
	std::string responseUtf8 = Converter::ansi_to_utf8(buf.str());
	try{
		sock.sendFrame(&responseUtf8[0], static_cast<int>(responseUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
	}
	catch (std::exception e){}
}

void CChatService::RunService_Make(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	// 채팅방을 생성한다.
	boost::property_tree::ptree responsePT;
	responsePT.put("event", "make");

	boost::optional<std::string> roomName = pt.get_optional<std::string>("data.room_name");
	boost::optional<std::string> userName = pt.get_optional<std::string>("data.user_name");
	boost::optional<const boost::property_tree::ptree&> inviteList = pt.get_child_optional("data.invite_list");
	std::shared_ptr<CChatRoom> spRoom;
	if (userName && inviteList)
	{
		// ---------------------------------------------------------------------
		// 채팅방을 찾는다.
		// ---------------------------------------------------------------------
		for each(auto room in m_roomContainer)
		{
			if (room.first.compare("all") == 0)
				continue;

			CChatRoom::UserContainer& uc = room.second->GetUserContainer();
			if (uc.find(userName.get()) == uc.end())
				continue;

			if (inviteList.get().size() + 1 != uc.size()){
				continue;
			}
			bool isAllExist = true;
			for each(auto user in inviteList.get()){
				boost::optional<std::string> friendName = user.second.get_optional<std::string>("name");
				if (friendName){
					if (uc.find(friendName.get()) == uc.end()){
						isAllExist = false;
						break;
					}
				}
			}
			if (!isAllExist)
				continue;

			spRoom = room.second;
			break;
		}

		// ---------------------------------------------------------------------
		// 컨테이너(메모리)에서 채팅방을 찾지 못했으면 파일DB에서 찾아본다.
		// ---------------------------------------------------------------------
		if (!spRoom)
		{
			boost::optional<std::string> roonNameInDB = m_spChatRepository->SelectRoom(userName.get(), inviteList.get());
			if (roonNameInDB)
			{				
				// 채팅방을 생성한다.
				spRoom = std::make_shared<CChatRoom>(roonNameInDB.get());
				m_roomContainer.insert(std::make_pair(roonNameInDB.get(), spRoom));

				// 채팅방에 대화대상을 추가한다. (웹소켓은 아직 연결안됨)
				std::vector<CChatRepository::UserInfo> vecUsers;
				m_spChatRepository->SelectRoomUser(roonNameInDB.get(), vecUsers);
				for each(auto user in vecUsers){
					spRoom->AddWebSocket(std::get<0>(user), boost::none);
					spRoom->SetUserRoomNickName(std::get<0>(user), std::get<1>(user));
					spRoom->SetUserStartReadSeq(std::get<0>(user), std::get<2>(user));
					spRoom->SetUserLastReadSeq(std::get<0>(user), std::get<3>(user));
				}
				// 최근 메세지를 읽어온다.(from DB)
				m_spChatRepository->SelectMessage(spRoom.get(), roonNameInDB.get(), 0, LOAD_MESSAGE_COUNT);
			}
		}

		// ---------------------------------------------------------------------
		// 메모리 or DB 모두에서 채팅방을 찾지 못했으면 새로 만든다.
		// ---------------------------------------------------------------------
		if (!spRoom)
		{
			static int seq = 0;
			std::string roomName = m_spChatRepository->CreateRoomName();

			// 채팅방을 생성한다.
			spRoom = std::make_shared<CChatRoom>(roomName);
			m_roomContainer.insert(std::make_pair(roomName, spRoom));

			// 채팅방에 본인을 추가한다. (웹소켓은 아직 연결안됨)
			spRoom->AddWebSocket(userName.get(), boost::none);
			m_spChatRepository->InsertRoom(roomName, userName.get(), 0);

			// 채팅방에 대화대상을 추가한다. (웹소켓은 아직 연결안됨)
			for each(auto user in inviteList.get()){
				boost::optional<std::string> friendName = user.second.get_optional<std::string>("name");
				if (friendName){
					spRoom->AddWebSocket(friendName.get(), boost::none);
					// DB에 저장한다.
					m_spChatRepository->InsertRoom(roomName, friendName.get(), 0);
				}
			}
		}
	}
	else if (roomName && userName)
	{
		// ---------------------------------------------------------------------
		// 채팅방을 찾는다.
		// 무조건 존재해야 한다.
		// ---------------------------------------------------------------------
		auto iter = m_roomContainer.find(roomName.get());
		if (iter != m_roomContainer.end()){
			spRoom = iter->second;
		}
	}
	else
	{
		// 파라미터에 문제가 있는경우
		boost::property_tree::ptree dataPT;
		dataPT.put("result", "fail");
		dataPT.put("result_desc", "invalid parameter");
		responsePT.put_child("data", dataPT);
	}

	// ---------------------------------------------------------------------
	// 채팅방을 찾았으면, 채팅방을 연다.
	// ---------------------------------------------------------------------
	if (spRoom)
	{
		boost::optional<Poco::Net::WebSocket> sock = spRoom->FindWebSocket(userName.get());
		if (sock)
		{
			// 열려있는 채팅방에 알림을 보낸다.
			boost::property_tree::ptree messageDataPT;
			messageDataPT.put("result", "already");
			messageDataPT.put("result_desc", spRoom->GetRoomName() + " room is already opened");
			boost::property_tree::ptree messagePT;
			messagePT.put("event", "join");
			messagePT.put_child("data", messageDataPT);

			std::ostringstream messageBuf;
			write_json(messageBuf, messagePT);
			std::string messageUtf8 = Converter::ansi_to_utf8(messageBuf.str());
			try{
				sock->sendFrame(&messageUtf8[0], static_cast<int>(messageUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
			}
			catch (std::exception ex){
				// 메세지 전송을 실패했을때만 채팅방을 새로 연다.
				RunService_Make(spRoom->GetRoomName(), userName.get());
			}

			boost::property_tree::ptree dataPT;
			dataPT.put("result", "fail");
			dataPT.put("result_desc", spRoom->GetRoomName() + " room is already opened");
			responsePT.put_child("data", dataPT);
		}
		else
		{
			RunService_Make(spRoom->GetRoomName(), userName.get());
		}
	}

	// 처리 결과를 알려준다.
	std::ostringstream buf;
	write_json(buf, responsePT);
	std::string responseUtf8 = Converter::ansi_to_utf8(buf.str());
	try{
		sock.sendFrame(&responseUtf8[0], static_cast<int>(responseUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
	}
	catch (std::exception e){}
}

void CChatService::RunService_Invite(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	boost::optional<std::string> roomName = pt.get_optional<std::string>("data.room_name");
	boost::optional<const boost::property_tree::ptree&> inviteList = pt.get_child_optional("data.invite_list");
	if (roomName && inviteList)
	{
		auto room = m_roomContainer.find(roomName.get());
		if (room != m_roomContainer.end())
		{
			std::string addedFriendList;
			__int64 lseq = room->second->GetLastSeq();
			for each(auto user in inviteList.get()){
				boost::optional<std::string> friendName = user.second.get_optional<std::string>("name");
				if (friendName){
					room->second->AddWebSocket(friendName.get(), boost::none);
					room->second->SetUserStartReadSeq(friendName.get(), lseq);
					if (!addedFriendList.empty()) addedFriendList += ", ";
					addedFriendList += friendName.get();
					
					m_spChatRepository->InsertRoom(roomName.get(), friendName.get(), lseq);
				}
			}

			boost::property_tree::ptree invitePT;
			invitePT.put("event", "invite");
			boost::property_tree::ptree dataPT;
			dataPT.put("result", "success");
			boost::property_tree::ptree userDataPT;
			// 채팅방의 유저목록을 만든다.
			for each(auto user in room->second->GetUserContainer()){
				boost::property_tree::ptree userPT;
				userPT.put("name", user.first);
				userDataPT.push_back(std::make_pair("", userPT));
			}
			dataPT.put_child("users", userDataPT);
			invitePT.put_child("data", dataPT);
			
			std::ostringstream buf;
			write_json(buf, invitePT);
			std::string inviteUtf8 = Converter::ansi_to_utf8(buf.str());

			for each(auto sock in room->second->GetUserContainer()){				
				if (sock.second){
					try{
						sock.second->sendFrame(&inviteUtf8[0], static_cast<int>(inviteUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
					}catch (std::exception ex){}
				}
			}
			
			boost::property_tree::ptree messageDataPT;
			messageDataPT.put("room_name", room->first);
			messageDataPT.put("user_name", "SYSTEM");
			messageDataPT.put("content", addedFriendList + "%20%EA%B0%80%20%EC%B4%88%EB%8C%80%20%EB%90%98%EC%97%88%EC%8A%B5%EB%8B%88%EB%8B%A4.");
			boost::property_tree::ptree messagePT;
			messagePT.put_child("data", messageDataPT);
			RunService_Message(messagePT, boost::none);
		}
	}
}

void CChatService::RunService_Join(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	boost::property_tree::ptree responsePT;
	responsePT.put("event", "join");

	boost::optional<std::string> roomName = pt.get_optional<std::string>("data.room_name");
	boost::optional<std::string> userName = pt.get_optional<std::string>("data.user_name");
	if (roomName && userName)
	{
		auto room = m_roomContainer.find(roomName.get());
		if (room != m_roomContainer.end())
		{
			// 채팅방이 존재한다.
			boost::property_tree::ptree dataPT;
			dataPT.put("result", "success");
			boost::property_tree::ptree userDataPT;
			// 채팅방의 유저목록을 만든다.
			for each(auto user in room->second->GetUserContainer()){
				boost::property_tree::ptree userPT;
				userPT.put("name", user.first);
				userPT.put("room_nick_name", room->second->GetUserRoomNickName(user.first));
				userDataPT.push_back(std::make_pair("", userPT));
			}
			dataPT.put_child("users", userDataPT);
			responsePT.put_child("data", dataPT);

			// 채팅방에 저장되어 있는 본인의 웹소켓을 업데이트 한다.
			room->second->AddWebSocket(userName.get(), sock);

			// 과거메세지를 전송한다.
			std::string allMessage;
			__int64 firstSeq = room->second->GetFirstSeq();
			__int64 lastSeq = room->second->GetLastSeq();
			__int64 userLastSeq = room->second->GetUserLastReadSeq(userName.get());
			if (userLastSeq < firstSeq){
				// 메모리에 없다 --> DB에서 읽어온다.
				m_spChatRepository->SelectMessage(room->second.get(), roomName.get(), firstSeq, LOAD_MESSAGE_COUNT);
			}
			__int64 needCount = lastSeq - userLastSeq;
			__int64 count = room->second->GetMessages(allMessage, lastSeq, needCount > MORE_MESSAGE_COUNT ? needCount : MORE_MESSAGE_COUNT);
			if (count > 0){
				try{
					sock.sendFrame(&allMessage[0], static_cast<int>(allMessage.size()), Poco::Net::WebSocket::FRAME_TEXT);
				}
				catch (std::exception ex){}
				RunService_Room(roomName.get(), "join");
			}
		}
		else
		{
			// 채팅방이 존재하지 않는다.
			boost::property_tree::ptree dataPT;
			dataPT.put("result", "fail");
			dataPT.put("result_desc", roomName.get() + " room is closed");
			responsePT.put_child("data", dataPT);
		}
	}
	else
	{
		// 파라미터에 문제가 있는경우
		boost::property_tree::ptree dataPT;
		dataPT.put("result", "fail");
		dataPT.put("result_desc", "invalid parameter");
		responsePT.put_child("data", dataPT);
	}

	// 처리 결과를 알려준다.
	std::ostringstream buf;
	write_json(buf, responsePT);
	std::string responseUtf8 = Converter::ansi_to_utf8(buf.str());
	try{
		sock.sendFrame(&responseUtf8[0], static_cast<int>(responseUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
	}
	catch (std::exception e){}
}

void CChatService::RunService_Leave(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	boost::optional<std::string> roomName = pt.get_optional<std::string>("data.room_name");
	boost::optional<std::string> userName = pt.get_optional<std::string>("data.user_name");
	if (roomName && userName)
	{
		std::shared_ptr<CChatRoom> spRoom;
		auto room = m_roomContainer.find(roomName.get());
		if (room == m_roomContainer.end())
		{
			// 없으면 만든다.
			spRoom = std::make_shared<CChatRoom>(roomName.get());
			m_roomContainer.insert(std::make_pair(roomName.get(), spRoom));

			// 채팅방에 대화대상을 추가한다. (웹소켓은 아직 연결안됨)
			std::vector<CChatRepository::UserInfo> vecUsers;
			m_spChatRepository->SelectRoomUser(roomName.get(), vecUsers);
			for each(auto user in vecUsers){
				spRoom->AddWebSocket(std::get<0>(user), boost::none);
				spRoom->SetUserRoomNickName(std::get<0>(user), std::get<1>(user));
				spRoom->SetUserStartReadSeq(std::get<0>(user), std::get<2>(user));
				spRoom->SetUserLastReadSeq(std::get<0>(user), std::get<3>(user));
			}
		}
		else{
			spRoom = room->second;
		}

		if (spRoom)
		{
			spRoom->DeleteWebSocket(userName.get());
			m_spChatRepository->DeleteRoom(roomName.get(), userName.get());

			boost::property_tree::ptree invitePT;
			invitePT.put("event", "invite");
			boost::property_tree::ptree dataPT;
			dataPT.put("result", "success");
			boost::property_tree::ptree userDataPT;
			// 채팅방의 유저목록을 만든다.
			for each(auto user in spRoom->GetUserContainer()){
				boost::property_tree::ptree userPT;
				userPT.put("name", user.first);
				userDataPT.push_back(std::make_pair("", userPT));
			}
			dataPT.put_child("users", userDataPT);
			invitePT.put_child("data", dataPT);

			std::ostringstream buf;
			write_json(buf, invitePT);
			std::string inviteUtf8 = Converter::ansi_to_utf8(buf.str());

			for each(auto sock in spRoom->GetUserContainer()){
				if (sock.second){
					try{
						sock.second->sendFrame(&inviteUtf8[0], static_cast<int>(inviteUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
					}
					catch (std::exception ex){}
				}
			}

			boost::property_tree::ptree messageDataPT;
			messageDataPT.put("room_name", roomName.get());
			messageDataPT.put("user_name", "SYSTEM");
			messageDataPT.put("content", userName.get() + "%20%EA%B0%80%20%EC%B1%84%ED%8C%85%EB%B0%A9%EC%9D%84%20%EB%82%98%EA%B0%94%EC%8A%B5%EB%8B%88%EB%8B%A4.");
			boost::property_tree::ptree messagePT;
			messagePT.put_child("data", messageDataPT);
			RunService_Message(messagePT, boost::none);

			std::string roomsMessage;
			RunService_RoomListMake(userName.get(), roomsMessage);
			try{
				auto room = m_roomContainer.find("all");
				auto sock = room->second->FindWebSocket(userName.get());
				sock->sendFrame(&roomsMessage[0], static_cast<int>(roomsMessage.size()), Poco::Net::WebSocket::FRAME_TEXT);
			}catch (std::exception e){}
		}
	}
}

void CChatService::RunService_Users(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	std::string responseUtf8;
	RunService_UserListMake("all", responseUtf8);
	try{
		sock.sendFrame(&responseUtf8[0], static_cast<int>(responseUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
	}
	catch (std::exception e){}
}

void CChatService::RunService_Message(const boost::property_tree::ptree& pt, boost::optional<Poco::Net::WebSocket> sock)
{
	boost::property_tree::ptree responsePT;
	responsePT.put("event", "message");

	boost::optional<std::string> roomName = pt.get_optional<std::string>("data.room_name");
	boost::optional<std::string> userName = pt.get_optional<std::string>("data.user_name");
	boost::optional<std::string> content = pt.get_optional<std::string>("data.content");
	if (roomName && userName && content)
	{
		auto room = m_roomContainer.find(roomName.get());
		if (room != m_roomContainer.end())
		{
			// 채팅방이 존재한다.
			__int64 curSeq = room->second->GetLastSeq() + 1;

			// 현재 시간을 구한다.
			std::ostringstream nowTime;
			const boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
			boost::posix_time::time_facet*const f = new boost::posix_time::time_facet("%Y-%m-%d %H:%M:%S");
			nowTime.imbue(std::locale(nowTime.getloc(), f));
			nowTime << now;

			// 메세지를 저장한다.
			boost::property_tree::ptree messageDataPT;
			messageDataPT.put("result", "success");
			messageDataPT.put("user_name", userName.get());
			messageDataPT.put("type", "message");
			messageDataPT.put("seq", boost::lexical_cast<std::string>(curSeq));
			messageDataPT.put("content", content.get());
			messageDataPT.put("time", nowTime.str());
			boost::property_tree::ptree messagePT;
			messagePT.put("event", "message");
			messagePT.put_child("data", messageDataPT);

			std::shared_ptr<CChatMessage> chatMsg = std::make_shared<CChatMessage>(curSeq, userName.get(), content.get(), nowTime.str(), messageDataPT);
			room->second->AddMessage(chatMsg);

			// DB에 저장한다.
			m_spChatRepository->InsertMessage(roomName.get(), userName.get(), content.get(), curSeq, nowTime.str());

			std::ostringstream messageBuf;
			write_json(messageBuf, messagePT);
			std::string messageUtf8 = Converter::ansi_to_utf8(messageBuf.str());

			// 채팅방에 접속해 있는 모두에게 메세지를 보낸다.
			std::string alarmMessage;
			RunService_AlarmMake(room->first, alarmMessage);
			for each(auto user in room->second->GetUserContainer())
			{
				if (user.second)
				{
					try{
						// 상대방이 채팅방에 접속해 있다면, 채팅방에 메세지를 보낸다.
						user.second->sendFrame(&messageUtf8[0], static_cast<int>(messageUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
					}
					catch (std::exception ex){
						// 상대방이 채팅방에 접속해있지 않다면, 알람을 준다.					
						RunService_AlarmSend(room->first, user.first, alarmMessage);
					}
				}
				else
				{
					// 상대방이 채팅방에 접속해있지 않다면, 알람을 준다.			
					RunService_AlarmSend(room->first, user.first, alarmMessage);
				}
			}
			RunService_Room(room->first, "message");
			return;
		}
		else
		{
			// 채팅방이 존재하지 않는다.
			boost::property_tree::ptree dataPT;
			dataPT.put("result", "fail");
			dataPT.put("result_desc", roomName.get() + " room is closed");
			responsePT.put_child("data", dataPT);
		}
	}
	else
	{
		// 파라미터에 문제가 있는경우
		boost::property_tree::ptree dataPT;
		dataPT.put("result", "fail");
		dataPT.put("result_desc", "invalid parameter");
		responsePT.put_child("data", dataPT);
	}

	// 처리 결과를 알려준다.
	if (sock){
		std::ostringstream buf;
		write_json(buf, responsePT);
		std::string responseUtf8 = Converter::ansi_to_utf8(buf.str());
		try{
			sock.get().sendFrame(&responseUtf8[0], static_cast<int>(responseUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
		}
		catch (std::exception e){}
	}
}

void CChatService::RunService_More(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	boost::optional<std::string> roomName = pt.get_optional<std::string>("data.room_name");
	boost::optional<std::string> startSeq = pt.get_optional<std::string>("data.start_seq");
	if (roomName && startSeq)
	{
		auto iter = m_roomContainer.find(roomName.get());
		if (iter != m_roomContainer.end())
		{
			auto room = iter->second;
			__int64 firstSeq = room->GetFirstSeq();
			__int64 startSeq2 = boost::lexical_cast<__int64>(startSeq.get());
			if (startSeq2 < firstSeq && startSeq2 > 0){
				// 메모리에 없다 --> DB에서 읽어온다.
				m_spChatRepository->SelectMessage(room.get(), roomName.get(), startSeq2, LOAD_MESSAGE_COUNT);
			}
			std::string moreMessage;
			__int64 count = room->GetMessages(moreMessage, startSeq2, MORE_MESSAGE_COUNT);
			if (count > 0){
				try{
					sock.sendFrame(&moreMessage[0], static_cast<int>(moreMessage.size()), Poco::Net::WebSocket::FRAME_TEXT);
				}
				catch (std::exception ex){}
			}
		}
	}
}

void CChatService::RunService_Seq(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	boost::optional<std::string> roomName = pt.get_optional<std::string>("data.room_name");
	boost::optional<std::string> userName = pt.get_optional<std::string>("data.user_name");
	boost::optional<std::string> seq = pt.get_optional<std::string>("data.seq");
	if (roomName && userName && seq)
	{
		auto room = m_roomContainer.find(roomName.get());
		if (room != m_roomContainer.end())
		{
			// 채팅방에서 읽었음으로 마지막시퀀스번호를 갱신한다.
			__int64 lastReadSeq = boost::lexical_cast<__int64>(seq.get());
			__int64 userLastReadSeq = room->second->GetUserLastReadSeq(userName.get());
			if (lastReadSeq > userLastReadSeq){
				room->second->SetUserLastReadSeq(userName.get(), lastReadSeq);
				m_spChatRepository->UpdateRoom(roomName.get(), userName.get(), lastReadSeq);
				RunService_Room(room->first, "seq");
			}
		}
	}
}

void CChatService::RunService_Close(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	auto room = m_roomContainer.find("all");
	auto userIter = room->second->GetUserContainer().begin();
	for (; userIter != room->second->GetUserContainer().end();)
	{
		if (userIter->second)
		{
			if (userIter->second.get() != sock)
			{
				userIter++;
			}
			else
			{
				userIter = room->second->GetUserContainer().erase(userIter);
			}
		}
		else
		{
			userIter++;
		}
	}
	RunService_Users();
}

void CChatService::RunService_RoomName(const boost::property_tree::ptree& pt, Poco::Net::WebSocket sock)
{
	boost::optional<std::string> roomName = pt.get_optional<std::string>("data.room_name");
	boost::optional<std::string> userName = pt.get_optional<std::string>("data.user_name");
	boost::optional<std::string> roomNickName = pt.get_optional<std::string>("data.room_nick_name");
	if (roomName && userName && roomNickName)
	{
		// 메모리 값 변경
		std::shared_ptr<CChatRoom> spRoom;
		auto room = m_roomContainer.find(roomName.get());
		if (room == m_roomContainer.end())
		{
			// 없으면 만든다.
			spRoom = std::make_shared<CChatRoom>(roomName.get());
			m_roomContainer.insert(std::make_pair(roomName.get(), spRoom));

			// 채팅방에 대화대상을 추가한다. (웹소켓은 아직 연결안됨)
			std::vector<CChatRepository::UserInfo> vecUsers;
			m_spChatRepository->SelectRoomUser(roomName.get(), vecUsers);
			for each(auto user in vecUsers){
				spRoom->AddWebSocket(std::get<0>(user), boost::none);
				spRoom->SetUserRoomNickName(std::get<0>(user), roomName.get());
				spRoom->SetUserStartReadSeq(std::get<0>(user), std::get<2>(user));
				spRoom->SetUserLastReadSeq(std::get<0>(user), std::get<3>(user));
			}
		}
		else{
			spRoom = room->second;
		}

		if (spRoom)
		{
			// 메모리값 변경
			spRoom->SetUserRoomNickName(userName.get(), roomNickName.get());

			// DB 값 변경
			bool bRet = m_spChatRepository->UpdateRoom(roomName.get(), userName.get(), roomNickName.get());

			boost::property_tree::ptree roomNameDataPT;
			roomNameDataPT.put("result", "success");
			roomNameDataPT.put("user_name", userName.get());
			roomNameDataPT.put("room_name", roomName.get());
			roomNameDataPT.put("room_nick_name", roomNickName.get());
			boost::property_tree::ptree roomNamePT;
			roomNamePT.put("event", "roomname");
			roomNamePT.put_child("data", roomNameDataPT);

			std::ostringstream buf;
			write_json(buf, roomNamePT);
			std::string responseUtf8 = Converter::ansi_to_utf8(buf.str());

			// 변경내역을 메인창에 전송한다.
			try{
				sock.sendFrame(&responseUtf8[0], static_cast<int>(responseUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
			}
			catch (std::exception e){}

			// 변경내역을 채팅방에 전송한다.
			auto user = spRoom->GetUserContainer().find(userName.get());
			boost::optional<Poco::Net::WebSocket> sock = user->second;
			if (sock){
				try{
					user->second->sendFrame(&responseUtf8[0], static_cast<int>(responseUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
				}
				catch (std::exception e){}
			}
		}
	}
}

void CChatService::RunService_Users()
{
	std::string responseUtf8;
	RunService_UserListMake("all", responseUtf8);
	for each(auto user in m_roomContainer.find("all")->second->GetUserContainer()){
		try{
			user.second->sendFrame(&responseUtf8[0], static_cast<int>(responseUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
		}
		catch (std::exception ex){}
	}
}

void CChatService::RunService_UserListMake(const std::string roomName, std::string& usersMessage)
{
	boost::property_tree::ptree responsePT;
	responsePT.put("event", "users");

	boost::property_tree::ptree dataPT;
	for each(auto user in m_roomContainer.find(roomName)->second->GetUserContainer()){
		boost::property_tree::ptree userPT;
		userPT.put("name", user.first);
		dataPT.push_back(std::make_pair("", userPT));
	}
	responsePT.put_child("data", dataPT);

	// 처리 결과를 알려준다.
	std::ostringstream buf;
	write_json(buf, responsePT);
	usersMessage = Converter::ansi_to_utf8(buf.str());
}

bool CChatService::RunService_AlarmMake(const std::string& roomName, std::string& alarmMessage)
{
	CChatRoom::UserContainer& uc = m_roomContainer.find(roomName)->second->GetUserContainer();
	auto iter = m_roomContainer.find(roomName);
	if (iter == m_roomContainer.end()){
		return false;
	}

	auto tpl = iter->second->GetLastMessage();
	if (tpl)
	{
		boost::property_tree::ptree alarmDataPT;
		alarmDataPT.put("last_sender", std::get<0>(tpl.get()));
		alarmDataPT.put("last_message", std::get<1>(tpl.get()));
		alarmDataPT.put("last_time", std::get<2>(tpl.get()));
		alarmDataPT.put("room_name", roomName);
		boost::property_tree::ptree alarmPT;
		alarmPT.put("event", "alarm");
		alarmPT.put_child("data", alarmDataPT);

		std::ostringstream alarmBuf;
		write_json(alarmBuf, alarmPT);
		alarmMessage = std::move(Converter::ansi_to_utf8(alarmBuf.str()));
	}
	else{
		return false;
	}
	
	return true;
}

void CChatService::RunService_AlarmSend(const std::string& roomName, const std::string& userName, const std::string& alarmMessage)
{
	auto userSock = m_roomContainer.find("all")->second->FindWebSocket(userName);
	if (userSock){
		try{
			userSock->sendFrame(&alarmMessage[0], static_cast<int>(alarmMessage.size()), Poco::Net::WebSocket::FRAME_TEXT);
		}
		catch (std::exception e){}
	}
}

void CChatService::RunService_Room(const std::string& roomName, const std::string& type)
{
	auto room = m_roomContainer.find(roomName);
	if (room == m_roomContainer.end()){
		return;
	}

	CChatRoom::UserContainer& uc = room->second->GetUserContainer();
	auto tpl = room->second->GetLastMessage();

	boost::property_tree::ptree roomUsersDataPT;
	for each(auto user in uc){
		boost::property_tree::ptree userPT;
		userPT.put("name", user.first);
		userPT.put("room_nick_name", room->second->GetUserRoomNickName(user.first));
		userPT.put("start_read_seq", room->second->GetUserStartReadSeq(user.first));
		userPT.put("last_read_seq", room->second->GetUserLastReadSeq(user.first));
		roomUsersDataPT.push_back(std::make_pair("", userPT));
	}
	boost::property_tree::ptree roomDataPT;
	roomDataPT.put("result", "success");
	roomDataPT.put("type", type);
	roomDataPT.put("room_name", roomName);
	roomDataPT.put_child("room_users", roomUsersDataPT);
	roomDataPT.put("last_sender", std::get<0>(tpl.get()));
	roomDataPT.put("last_message", std::get<1>(tpl.get()));
	roomDataPT.put("last_time", std::get<2>(tpl.get()));
	roomDataPT.put("last_message_seq", room->second->GetLastSeq());
	boost::property_tree::ptree roomPT;
	roomPT.put("event", "room");
	roomPT.put_child("data", roomDataPT);

	std::ostringstream roomBuf;
	write_json(roomBuf, roomPT);
	std::string roomUtf8 = Converter::ansi_to_utf8(roomBuf.str());

	for each(auto user in uc)
	{
		auto userSock = m_roomContainer.find("all")->second->FindWebSocket(user.first);
		if (userSock){
			try{
				// 메인창의 채팅목록 리스트를 갱신
				userSock->sendFrame(&roomUtf8[0], static_cast<int>(roomUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
			}
			catch (std::exception ex){}
		}
		try{
			// 채팅방의 언리드카운트를 갱신
			if (user.second){
				user.second->sendFrame(&roomUtf8[0], static_cast<int>(roomUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
			}
		}
		catch (std::exception ex){}
	}
}

void CChatService::RunService_RoomListMake(const std::string& userName, std::string& roomsMessage)
{
	// tuple(roomName, userName, message, messageSeq, sendTime);
	std::vector<CChatRepository::MessageInfo> vecMessages;
	m_spChatRepository->SelectLastMessagePerRoom(userName, vecMessages);
	
	boost::property_tree::ptree roomArrayPT;
	for each(auto msg in vecMessages)
	{
		std::vector<CChatRepository::UserInfo> vecUsers;
		m_spChatRepository->SelectRoomUser(std::get<0>(msg), vecUsers);

		boost::property_tree::ptree roomUsersDataPT;
		for each(auto user in vecUsers)
		{
			boost::property_tree::ptree userPT;
			userPT.put("name", std::get<0>(user));
			userPT.put("room_nick_name", std::get<1>(user));
			userPT.put("start_read_seq", std::get<2>(user));			
			userPT.put("last_read_seq", std::get<3>(user));
			roomUsersDataPT.push_back(std::make_pair("", userPT));
		}

		boost::property_tree::ptree roomDataPT;
		roomDataPT.put("room_name", std::get<0>(msg));		
		roomDataPT.put_child("room_users", roomUsersDataPT);
		roomDataPT.put("last_sender", std::get<1>(msg));
		roomDataPT.put("last_message", std::get<2>(msg));
		roomDataPT.put("last_message_seq", std::get<3>(msg));
		roomDataPT.put("last_time", std::get<4>(msg));

		roomArrayPT.push_back(std::make_pair("", roomDataPT));
	}

	boost::property_tree::ptree roomPT;
	roomPT.put("event", "room_s");
	roomPT.put_child("data", roomArrayPT);

	std::ostringstream alarmBuf;
	write_json(alarmBuf, roomPT);
	roomsMessage = std::move(Converter::ansi_to_utf8(alarmBuf.str()));
}

void CChatService::RunService_Make(const std::string& roomName, const std::string& userName)
{
	boost::property_tree::ptree makeDataPT;
	makeDataPT.put("result", "success");
	makeDataPT.put("path", "/chat_room");
	makeDataPT.put("room_name", roomName);
	boost::property_tree::ptree makePT;
	makePT.put("event", "make");
	makePT.put_child("data", makeDataPT);

	std::ostringstream makeBuf;
	write_json(makeBuf, makePT);
	std::string makeUtf8 = Converter::ansi_to_utf8(makeBuf.str());

	auto userSock = m_roomContainer.find("all")->second->FindWebSocket(userName);
	if (userSock){
		try{
			userSock->sendFrame(&makeUtf8[0], static_cast<int>(makeUtf8.size()), Poco::Net::WebSocket::FRAME_TEXT);
		}
		catch (std::exception ex){}
	}
}