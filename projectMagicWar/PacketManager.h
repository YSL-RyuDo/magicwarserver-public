#pragma once
#include <unordered_map>
#include <string>
#include "PacketType.h"
#include "PacketStruct.h"

class Session;
class MatchManager;
class IOCP_Send;

class PacketManager
{
public:
    static void Init(std::unordered_map<std::string, Session*>* userMap,
        MatchManager* matchMgr,
        IOCP_Send* sendProc);

    // 패킷 파싱 호출
    static void HandlePacket(Session* session, int packetId, const char* body, int bodySize);

    // 바이너리 전송
    static void SendPacket(Session* session, uint16_t packetId, const void* data, uint16_t dataSize);

private:
    // 개별 패킷 핸들러
    static void HandleSignup(Session* s, const char* body, int size);
    static void HandleLogin(Session* s, const char* body, int size);

    // 응답 함수
    static void SendSignupAck(Session* s, uint16_t result);
    static void SendLoginAck(Session* s, uint16_t result, const std::string& nick);

private:
    static std::unordered_map<std::string, Session*>* s_userMap;
    static MatchManager* s_matchMgr;
    static IOCP_Send* s_sendProc;
};
