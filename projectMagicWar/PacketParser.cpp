#include "PacketParser.h"
#include "PacketManager.h"
#include <iostream>
#include <cstring>

void PacketParser::ProcessBuffer(Session* session)
{
    char* buf = session->recvBuffer;
    int& used = session->recvUsed;

    while (true)
    {
        // 헤더(8바이트) 미만이면 패킷 불완성
        if (used < (int)sizeof(PACKET_HEADER))
            return;

        PACKET_HEADER header;
        memcpy(&header, buf, sizeof(PACKET_HEADER));

        int totalSize = sizeof(PACKET_HEADER) + header.bodySize;

        // 전체 패킷이 다 도착했는지 확인
        if (used < totalSize)
            return;

        // 토큰 검증: 로그인 전(token=0)에는 무시
        if (session->token != 0 && header.token != session->token)
        {
            std::cout << "[보안] 토큰 불일치! session=" << session->sessionId << "\n";
            return;
        }

        // 패킷 바디
        const char* body = buf + sizeof(PACKET_HEADER);

        // 패킷 처리
        PacketManager::HandlePacket(session, header.packetId, body, header.bodySize);

        // 버퍼에서 패킷 제거
        memmove(buf, buf + totalSize, used - totalSize);
        used -= totalSize;
    }
}
