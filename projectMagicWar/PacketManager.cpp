#include "PacketManager.h"

#include "IOCP_Send.h"
#include "UserManager.h"
#include "PasswordHasher.h"
#include "MatchManager.h"

#include <iostream>
#include <random>
#include <vector>
#include <cstring>
#include <algorithm>

// ---------------------------
// static members
// ---------------------------
std::unordered_map<std::string, Session*>* PacketManager::s_userMap = nullptr;
MatchManager* PacketManager::s_matchMgr = nullptr;
IOCP_Send* PacketManager::s_sendProc = nullptr;
std::mutex PacketManager::s_userMapMtx;
// ---------------------------
// util
// ---------------------------
static uint32_t GenerateRandomToken()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis;
    return dis(gen);
}

// ---------------------------
// Init
// ---------------------------
void PacketManager::Init(
    std::unordered_map<std::string, Session*>* userMap,
    MatchManager* matchMgr,
    IOCP_Send* sendProc)
{
    s_userMap = userMap;
    s_matchMgr = matchMgr;
    s_sendProc = sendProc;
}

// ---------------------------
// Packet Entry
// ---------------------------
void PacketManager::HandlePacket(
    Session* session,
    int packetId,
    const char* body,
    int bodySize)
{
    switch (packetId)
    {
    case PKT_SIGNUP:
        HandleSignup(session, body, bodySize);
        break;

    case PKT_LOGIN:
        HandleLogin(session, body, bodySize);
        break;
    case PKT_LOGOUT:
        HandleLogout(session);
        break;
    case PKT_REQ_PROFILE:
        HandleProfileRequest(session, body, bodySize);
        break;
    case PKT_CHAT_LOBBY:
        HandleLobbyChat(session, body, bodySize);
        break;

    default:
        std::cout << "[WARN] Unknown packetId=" << packetId << "\n";
        break;
    }
}

// ---------------------------
// Send Packet
// ---------------------------
void PacketManager::SendPacket(
    Session* session,
    uint16_t packetId,
    const void* data,
    uint16_t dataSize)
{
    if (!session || session->closing) return;

    PACKET_HEADER header{};
    header.packetId = packetId;
    header.token = session->token;
    header.bodySize = dataSize;

    std::vector<char> buf(sizeof(PACKET_HEADER) + dataSize);
    memcpy(buf.data(), &header, sizeof(header));
    memcpy(buf.data() + sizeof(header), data, dataSize);

    s_sendProc->EnqueueSend(session, buf.data(), (int)buf.size());
}

void PacketManager::HandleSessionDisconnect(Session* s)
{
    if (!s) return;

    if (!s->userId.empty())
        UserManager::LogoutUser(s->userId);

    s->Reset();
}




// ---------------------------
// Signup
// ---------------------------
void PacketManager::HandleSignup(Session* s, const char* body, int size)
{
    if (size != sizeof(PKT_SIGNUP_REQ)) return;

    PKT_SIGNUP_REQ req{};
    memcpy(&req, body, sizeof(req));

    std::string id(req.id);
    std::string pw(req.pw);
    std::string nick(req.nick);

    if (UserManager::IsIDExist(id))
    {
        SendSignupAck(s, PKT_SIGNUP_ID_EXISTS);
        return;
    }

    if (UserManager::IsNickExist(nick))
    {
        SendSignupAck(s, PKT_SIGNUP_NICK_EXISTS);
        return;
    }

    std::string salt = PasswordHasher::GenerateSalt(16);
    std::string hash = PasswordHasher::Hash(pw, salt);

    UserManager::AddUser(id, hash, salt, nick);
    UserManager::CreateUserInfo(id);
    UserManager::CreateUserMagic(id);
    UserManager::CreateUserRank(id);

    SendSignupAck(s, PKT_SIGNUP_OK);
}

// ---------------------------
// Login
// ---------------------------
void PacketManager::HandleLogin(Session* s, const char* body, int size)
{
    if (size != sizeof(PKT_LOGIN_REQ))
        return;

    PKT_LOGIN_REQ req{};
    memcpy(&req, body, sizeof(req));

    std::string id(req.id);
    std::string pw(req.pw);

    std::string dbHash, dbSalt, nick;

    // 1️ ID 존재 여부
    if (!UserManager::LoadUser(id, dbHash, dbSalt, nick))
    {
        SendLoginAck(s, PKT_LOGIN_ID_FAIL);
        return;
    }

    // 2️ 비밀번호 검증
    if (PasswordHasher::Hash(pw, dbSalt) != dbHash)
    {
        SendLoginAck(s, PKT_LOGIN_PW_FAIL);
        return;
    }

    if (!UserManager::TryLoginUser(id))
    {
        std::cout << "[LOGIN FAIL] already logged in: " << id << "\n";
        SendLoginAck(s, PKT_LOGIN_ALREADY);
        return;
    }

    // 4️ 로그인 성공 처리
    s->userId = id;
    s->nickname = nick;
    s->authenticated = true;
    s->token = GenerateRandomToken();

    {
        std::lock_guard<std::mutex> lock(s_userMapMtx);
        (*s_userMap)[id] = s;
    }

    PKT_LOGIN_ACK ack{};
    ack.result = PKT_LOGIN_OK;
    ack.token = s->token;
    memcpy(ack.nick, nick.c_str(), std::min<size_t>(31, nick.size()));

    SendPacket(s, PKT_LOGIN_OK, &ack, sizeof(ack));

    std::cout << "[LOGIN OK] user=" << id << "\n";
}

// ---------------------------
// Profile
// ---------------------------
void PacketManager::HandleProfileRequest(Session* s, const char* body, int bodySize)
{
    std::cout << "[PROFILE] HandleProfileRequest 진입\n";

    if (!s)
    {
        std::cout << "[PROFILE] Session nullptr\n";
        return;
    }

    std::cout << "[PROFILE] bodySize = " << bodySize << "\n";

    if (bodySize != 32)
    {
        std::cout << "[PROFILE] bodySize 오류\n";
        return;
    }

    char idBuf[33]{};
    memcpy(idBuf, body, 32);
    std::string userId(idBuf);

    std::cout << "[PROFILE] 요청 userId = " << userId << "\n";
    std::cout << "[PROFILE] 세션 userId = " << s->userId << "\n";
    std::cout << "[PROFILE] authenticated = " << s->authenticated << "\n";

    std::string nickname;
    uint32_t level = 0, exp = 0;
    uint32_t rank = 0, rankScore = 0;

    if (!UserManager::LoadUserAccount(userId, nickname))
    {
        std::cout << "[PROFILE] LoadUserAccount 실패\n";
        return;
    }

    std::cout << "[PROFILE] nickname = " << nickname << "\n";

    UserManager::LoadUserInfo(userId, level, exp);
    UserManager::LoadUserRank(userId, rank, rankScore);

    std::cout << "[PROFILE] level=" << level
        << " exp=" << exp
        << " rank=" << rank
        << " score=" << rankScore << "\n";

    RankInfo rankInfo{};
    RankManager::GetRankInfo(rank, rankInfo);

    std::cout << "[PROFILE] rankName=" << rankInfo.name
        << " min=" << rankInfo.minScore
        << " max=" << rankInfo.maxScore << "\n";

    PKT_RES_PROFILE_DATA profile{};
    memcpy(profile.nickname, nickname.c_str(),
        std::min<size_t>(31, nickname.size()));

    profile.level = level;
    profile.exp = exp;
    profile.rank = rank;
    profile.rankScore = rankScore;

    memcpy(profile.rankName, rankInfo.name.c_str(),
        std::min<size_t>(15, rankInfo.name.size()));

    profile.minScore = rankInfo.minScore;
    profile.maxScore = rankInfo.maxScore;

    std::cout << "[PROFILE] 프로필 패킷 전송\n";

    SendPacket(s, PKT_RES_PROFILE, &profile, sizeof(profile));

    std::cout << "[PROFILE] HandleProfileRequest 종료\n";
}



void PacketManager::HandleLogout(Session* s)
{
    if (s->userId.empty())
        return;

    std::cout << "[LOGOUT] user=" << s->userId << "\n";

    UserManager::LogoutUser(s->userId);

    if (s_matchMgr)
        s_matchMgr->CancelMatch(s);

    s->authenticated = false;
    s->userId.clear();
    s->token = 0;
}


void PacketManager::HandleLobbyChat(Session* sender, const char* body, int bodySize)
{
    if (bodySize <= 0 || sender->inBattle)
        return;

    PKT_CHAT_LOBBY_DATA pkt{};
    strncpy_s(pkt.nickname, sender->nickname.c_str(), 31);
    memcpy(pkt.message, body, min(bodySize, 127));

    std::lock_guard<std::mutex> lock(s_userMapMtx);

    std::cout << "[LOBBY CHAT] 송신자 = "
        << sender->userId
        << " / 메시지 = "
        << pkt.message
        << std::endl;

    for (auto it = s_userMap->begin(); it != s_userMap->end(); ++it)
    {
        Session* session = it->second;

        if (!session || session->closing)
            continue;

        if (session->inBattle)
            continue;

        //  여기서 “누구에게 보내는지” 확인 가능
        std::cout << "  → 수신자: "
            << session->userId
            << " (token=" << session->token << ")"
            << std::endl;

        SendPacket(session, PKT_CHAT_LOBBY, &pkt, sizeof(pkt));
    }

    std::cout << "[LOBBY CHAT] 브로드캐스트 완료\n";
}



// ---------------------------
// Ack helpers
// ---------------------------
void PacketManager::SendSignupAck(Session* s, uint16_t result)
{
    PKT_SIGNUP_ACK ack{};
    ack.result = result;
    SendPacket(s, result, &ack, sizeof(ack));
}

void PacketManager::SendLoginAck(Session* s, uint16_t result)
{
    PKT_LOGIN_ACK ack{};
    ack.result = result;
    SendPacket(s, result, &ack, sizeof(ack));
}
