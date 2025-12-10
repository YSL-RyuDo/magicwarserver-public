#include "PacketParser.h"
#include "PacketManager.h"
#include <iostream>
#include <cstring>

static const int HEADER_SIZE = 4;

void PacketParser::ProcessBuffer(Session* session)
{
    int packetId = 0;
    std::string body;

    // 여러 패킷이 한 번에 들어올 수 있음
    while (ExtractPacket(session, packetId, body))
    {
        // 디버그 출력
        // std::cout << "[Packet OK] ID=" << packetId << " Body=" << body << std::endl;

        // PacketManager로 전달
        PacketManager::HandlePacket(session, packetId, body);
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
