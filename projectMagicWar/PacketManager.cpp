#include <iostream>

#include "PacketManager.h"
#include "UserManager.h"
#include "MatchManager.h"
#include "IOCP_Send.h"


std::unordered_map<std::string, Session*>* PacketManager::s_userMap = nullptr;
MatchManager* PacketManager::s_matchMgr = nullptr;
IOCP_Send* PacketManager::s_sendProc = nullptr;

//===========================================================
// 초기화
//===========================================================
void PacketManager::Init(std::unordered_map<std::string, Session*>* userMap,
    MatchManager* matchMgr,
    IOCP_Send* sendProc)
{
    s_userMap = userMap;
    s_matchMgr = matchMgr;
    s_sendProc = sendProc;
}

//===========================================================
// 패킷 전송
//===========================================================
void PacketManager::SendPacket(Session* session, int packetId, const std::string& msg)
{
    if (!session || session->closing) return;

    char header[4];
    memcpy(header, &packetId, 4);

    std::string packet;
    packet.append(header, 4);
    packet.append(msg);

    // ★ 객체 기반으로 변경
    s_sendProc->EnqueueSend(session, packet.c_str(), packet.size());
}

//===========================================================
// 수신 패킷 처리
//===========================================================
void PacketManager::HandlePacket(Session* session, int packetId, const std::string& body)
{
    std::cout << "[Packet] session=" << session->sessionId
        << " id=" << packetId
        << " body=" << body << std::endl;

    switch (packetId)
    {
        //---------------------------------------------------------
        // 회원가입
        //---------------------------------------------------------
    case PKT_SIGNUP:
    {
        size_t p1 = body.find(',');
        size_t p2 = body.find(',', p1 + 1);

        if (p1 == std::string::npos || p2 == std::string::npos)
        {
            SendPacket(session, PKT_SIGNUP_ID_EXISTS, "1");
            break;
        }

        std::string id = body.substr(0, p1);
        std::string pw = body.substr(p1 + 1, p2 - p1 - 1);
        std::string nick = body.substr(p2 + 1);

        if (UserManager::IsIDExist(id))
        {
            SendPacket(session, PKT_SIGNUP_ID_EXISTS, "2");
            break;
        }

        if (!UserManager::AddUser(id, pw, nick))
        {
            SendPacket(session, PKT_SIGNUP_NICK_EXISTS, "3");
            break;
        }

        SendPacket(session, PKT_SIGNUP_OK, nick);
        break;
    }

    //---------------------------------------------------------
    // 로그인
    //---------------------------------------------------------
    case PKT_LOGIN:
    {
        size_t pos = body.find(',');
        std::string id = body.substr(0, pos);
        std::string pw = body.substr(pos + 1);

        std::string nick;

        if (!UserManager::IsIDExist(id))
        {
            SendPacket(session, PKT_LOGIN_ID_FAIL, id);
            std::cout << "[로그인 실패] 세션 " << session->sessionId << " → ID 없음\n";
            break;
        }

        if (!UserManager::CheckLogin(id, pw, nick))
        {
            SendPacket(session, PKT_LOGIN_PW_FAIL, id);
            std::cout << "[로그인 실패] 세션 " << session->sessionId << " → PW 불일치\n";
            break;
        }

        session->authenticated = true;
        session->userId = id;
        session->nickname = nick;

        (*s_userMap)[id] = session;

        SendPacket(session, PKT_LOGIN_OK, nick);
        std::cout << "[로그인 성공] 세션 " << session->sessionId << " → nick=" << nick << "\n";
        break;
    }

    //---------------------------------------------------------
    // 프로필 요청
    //---------------------------------------------------------
    case PKT_REQ_PROFILE:
    {
        std::string result = "nick,level,rank,score";
        SendPacket(session, PKT_RES_PROFILE, result);
        break;
    }

    //---------------------------------------------------------
    // 매칭 요청
    //---------------------------------------------------------
    case PKT_MATCH_REQUEST:
    {
        if (!session->authenticated)
        {
            SendPacket(session, PKT_MATCH_CANCEL_OK, "NotLogin");
            return;
        }

        s_matchMgr->RequestMatch(session);
        break;
    }

    case PKT_MATCH_CANCEL:
        s_matchMgr->CancelMatch(session);
        break;

        //---------------------------------------------------------
        // 이동 브로드캐스트
        //---------------------------------------------------------
    case PKT_MOVE:
    {
        for (auto& kv : *s_userMap)
        {
            Session* target = kv.second;
            if (target == session) continue;

            SendPacket(target, PKT_MOVE, session->userId + "," + body);
        }
        break;
    }

    //---------------------------------------------------------
    default:
        std::cout << "[알 수 없는 패킷] id=" << packetId << std::endl;
        break;
    }
}
