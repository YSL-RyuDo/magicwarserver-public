#pragma once
#include <unordered_map>
#include <string>
#include<mutex>
#include "PacketType.h"
#include "PacketStruct.h"
#include "RankManager.h"

class Session;
class MatchManager;
class IOCP_Send;

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
        const char* body,
        int bodySize);

    static void SendPacket(
        Session* session,
        uint16_t packetId,
        const void* data,
        uint16_t dataSize);

    static void HandleSessionDisconnect(Session* session);

private:
    static void HandleSignup(Session* s, const char* body, int size);
    static void HandleLogin(Session* s, const char* body, int size);
    static void HandleProfileRequest(Session* s, const char* body, int bodySize);
    static void HandleLogout(Session* s);
    static void HandleLobbyChat(Session* sender, const char* body, int bodySize);
    static void SendSignupAck(Session* s, uint16_t result);
    static void SendLoginAck(Session* s, uint16_t result);
    
private:
    static std::unordered_map<std::string, Session*>* s_userMap;
    static MatchManager* s_matchMgr;
    static IOCP_Send* s_sendProc;
    static std::mutex s_userMapMtx;
};
