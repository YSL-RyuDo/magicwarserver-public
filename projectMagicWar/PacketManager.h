#pragma once
#include <unordered_map>
#include <string>
#include<mutex>
#include "PacketType.h"
#include "PacketStruct.h"

class Session;
class MatchManager;
class IOCP_Send;
class MagicManager;
using namespace std;

class PacketManager
{
public:
    static void Init(
        std::unordered_map<std::string, Session*>* userMap,
        MatchManager* matchMgr,
        IOCP_Send* sendProc);

    static void HandlePacket(
        Session* session,
        int packetId,
        uint32_t recvToken,
        const char* body,
        int bodySize);

    static void SendPacket(
        Session* session,
        uint16_t packetId,
        const void* data,
        uint16_t dataSize);

    static void HandleSessionDisconnect(Session* session);

private:
    static void HandlePING(Session* s);
    static void HandleSignup(Session* s, const char* body, int size);
    static void HandleLogin(Session* s, const char* body, int size);
    static void HandleProfileRequest(Session* s, const char* body, int bodySize);
    static void HandleLogout(Session* s);
    static void HandleLobbyChat(Session* sender, const char* body, int bodySize);
    static void HandleUserMagicRequest(Session* s);
    static void HandleUserMagicDetail(Session* s, const char* body, int bodySize);
    static void HandleReplaceUserMagic(Session* s, const char* body, int bodySize);
    static void HandleMove(Session* s, const char* body, int bodySize);
    static void HandleHit(Session* attacker, const char* body, int bodySize);
    static void HandleDie(Session* winner, Session* loser);
    static void HandleCastMagic(Session* s, const char* body, int size);
    static void HandleMagicDetailGAME(Session* s, const char* body, int bodySize);
    static void HandleCastFieldMagic(Session* s, const char* body, int bodySize);
    static void HandleCastSelfMagic(Session* session,const char* body,int bodySize);
    static void UpdateInvisibility(Session* session);
    static void SendSignupAck(Session* s, uint16_t result);
    static void SendLoginAck(Session* s, uint16_t result);
    static void SendHPResult(Session* s);
    static void SendSlow(Session* target, float ratio, int durationMs);
    static void SendStun(Session* target, int durationMs);
private:
    static std::unordered_map<std::string, Session*>* s_userMap;
    static MatchManager* s_matchMgr;
    static IOCP_Send* s_sendProc;
    static std::mutex s_userMapMtx;
};
