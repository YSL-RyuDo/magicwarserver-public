#include "PacketParser.h"
#include "PacketManager.h"
#include <iostream>
#include <cstring>

static const int HEADER_SIZE = 4;

void PacketParser::ProcessBuffer(Session* session)
{
    char* buf = session->recvBuffer;
    int& used = session->recvUsed;

    while (true)
    {
        if (used < 4)
            return;

        uint16_t pktId, bodySize;
        memcpy(&pktId, buf, 2);
        memcpy(&bodySize, buf + 2, 2);

        int total = 4 + bodySize;
        if (used < total)
            return;

        PacketManager::HandlePacket(session, pktId, buf + 4, bodySize);

        memmove(buf, buf + total, used - total);
        used -= total;
    }
}


bool PacketParser::ExtractPacket(Session* session, int& outPacketId, std::string& outBody)
{
    // 최소한 헤더 4바이트는 있어야 packetId 추출 가능
    if (session->recvUsed < HEADER_SIZE)
        return false;

    // packetId 추출
    memcpy(&outPacketId, session->recvBuffer, HEADER_SIZE);

    // body 시작 지점
    int bodyStart = HEADER_SIZE;

    // body의 길이는 정해져 있지 않고, recvUsed까지 모두 body임
    int bodyLen = session->recvUsed - HEADER_SIZE;
    if (bodyLen < 0)
        return false;

    outBody.assign(session->recvBuffer + bodyStart, bodyLen);

    // 패킷 하나를 소모했으므로 recvBuffer에서 제거(당기기)
    session->recvUsed = 0;

    return true;
}
