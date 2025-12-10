#pragma once
#include <unordered_map>
#include <string>

#include "PacketType.h"

class Session;
class MatchManager;
class IOCP_Send;

class PacketManager
{
public:
    static void Init(std::unordered_map<std::string, Session*>* userMap,
        MatchManager* matchMgr,
        IOCP_Send* sendProc);

    static void SendPacket(Session* session, int packetId, const std::string& msg);
    static void HandlePacket(Session* session, int packetId, const std::string& body);

private:
    static std::unordered_map<std::string, Session*>* s_userMap;
    static MatchManager* s_matchMgr;
    static IOCP_Send* s_sendProc;       // Send ´ã´ç °´Ã¼
};