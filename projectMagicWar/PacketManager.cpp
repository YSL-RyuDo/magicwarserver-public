#include <iostream>

#include "PacketManager.h"
#include "IOCP_Send.h"
#include "MatchManager.h"
#include "UserManager.h"
#include "PasswordHasher.h"

std::unordered_map<std::string, Session*>* PacketManager::s_userMap = nullptr;
MatchManager* PacketManager::s_matchMgr = nullptr;
IOCP_Send* PacketManager::s_sendProc = nullptr;

void PacketManager::Init(std::unordered_map<std::string, Session*>* userMap,
    MatchManager* matchMgr,
    IOCP_Send* sendProc)
{
    s_userMap = userMap;
    s_matchMgr = matchMgr;
    s_sendProc = sendProc;
}

void PacketManager::HandlePacket(Session* session, int packetId, const char* body, int bodySize)
{
    std::cout << "---------------------------------------------\n";
    std::cout << "[패킷] 도착 → session=" << session->sessionId
        << ", packetId=" << packetId
        << ", bodySize=" << bodySize << "\n";

    switch (packetId)
    {
    case PKT_SIGNUP:
        std::cout << "[회원가입] 요청 수신\n";
        HandleSignup(session, body, bodySize);
        break;

    case PKT_LOGIN:
        std::cout << "[로그인] 요청 수신\n";
        HandleLogin(session, body, bodySize);
        break;

    default:
        std::cout << "[패킷] Unknown packet ID=" << packetId << "\n";
        break;
    }
}

void PacketManager::SendPacket(Session* session, uint16_t pktId, const void* data, uint16_t dataSize)
{
    if (!session || session->closing) return;

    std::cout << "[전송] session=" << session->sessionId
        << ", pktId=" << pktId
        << ", size=" << dataSize << "\n";

    // 실제 전송
    uint16_t header[2];
    header[0] = pktId;
    header[1] = dataSize;

    std::vector<char> buffer;
    buffer.resize(4 + dataSize);

    memcpy(buffer.data(), header, 4);
    memcpy(buffer.data() + 4, data, dataSize);

    s_sendProc->EnqueueSend(session, buffer.data(), (int)buffer.size());
}

void PacketManager::HandleSignup(Session* s, const char* body, int size)
{
    std::cout << "[회원가입] HandleSignup() 시작\n";

    if (size != sizeof(PKT_SIGNUP_REQ))
    {
        std::cout << "[회원가입] 패킷 크기 오류 → size=" << size << "\n";
        return;
    }

    PKT_SIGNUP_REQ req{};
    memcpy(&req, body, sizeof(req));

    std::string id(req.id);
    std::string pw(req.pw);
    std::string nick(req.nick);

    std::cout << "[회원가입] 입력값 → ID=" << id
        << ", PW=" << pw
        << ", NICK=" << nick << "\n";

    if (UserManager::IsIDExist(id))
    {
        std::cout << "[회원가입] 실패 → ID 중복\n";
        SendSignupAck(s, PKT_SIGNUP_ID_EXISTS);
        return;
    }

    if (UserManager::IsNickExist(nick))
    {
        std::cout << "[회원가입] 실패 → Nick 중복\n";
        SendSignupAck(s, PKT_SIGNUP_NICK_EXISTS);
        return;
    }

    std::cout << "[회원가입] 암호화 및 저장 진행\n";

    std::string salt = PasswordHasher::GenerateSalt(16);
    std::string hash = PasswordHasher::Hash(pw, salt);

    UserManager::AddUser(id, hash, salt, nick);

    std::cout << "[회원가입] 성공 → 응답 전송\n";
    SendSignupAck(s, PKT_SIGNUP_OK);
}

void PacketManager::HandleLogin(Session* s, const char* body, int size)
{
    std::cout << "[로그인] HandleLogin() 시작\n";

    if (size != sizeof(PKT_LOGIN_REQ))
    {
        std::cout << "[로그인] 패킷 크기 오류 → size=" << size << "\n";
        return;
    }

    PKT_LOGIN_REQ req{};
    memcpy(&req, body, sizeof(req));

    std::string id(req.id);
    std::string pw(req.pw);

    std::cout << "[로그인] 입력값 → ID=" << id << ", PW=" << pw << "\n";

    std::string dbHash, dbSalt, nick;

    if (!UserManager::LoadUser(id, dbHash, dbSalt, nick))
    {
        std::cout << "[로그인] 실패 → ID 없음\n";
        SendLoginAck(s, PKT_LOGIN_ID_FAIL, "");
        return;
    }

    std::string computed = PasswordHasher::Hash(pw, dbSalt);

    if (computed != dbHash)
    {
        std::cout << "[로그인] 실패 → PW 불일치\n";
        SendLoginAck(s, PKT_LOGIN_PW_FAIL, "");
        return;
    }

    std::cout << "[로그인] 성공 → nick=" << nick << "\n";
    SendLoginAck(s, PKT_LOGIN_OK, nick);
}

void PacketManager::SendSignupAck(Session* s, uint16_t result)
{
    std::cout << "[회원가입] 응답 전송 → result=" << result << "\n";

    PKT_SIGNUP_ACK ack{};
    ack.result = result;

    SendPacket(s, result, &ack, sizeof(ack));
}

void PacketManager::SendLoginAck(Session* s, uint16_t result, const std::string& nick)
{
    std::cout << "[로그인] 응답 전송 → result=" << result
        << ", nick=" << nick << "\n";

    PKT_LOGIN_ACK ack{};
    ack.result = result;

    if (result == PKT_LOGIN_OK)
        memcpy(ack.nick, nick.c_str(), nick.size());

    SendPacket(s, result, &ack, sizeof(ack));
}
