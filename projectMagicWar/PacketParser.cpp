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
        // 1. 헤더 크기 미만이면 대기
        if (used < (int)sizeof(PACKET_HEADER))
        {
            std::cout << "[PARSER] 헤더 대기 중 used=" << used << "\n";
            return;
        }


        PACKET_HEADER header;
        memcpy(&header, buf, sizeof(PACKET_HEADER));

        std::cout
            << "[PARSER] packetId=" << header.packetId
            << " token=" << header.token
            << " bodySize=" << header.bodySize
            << " recvUsed=" << used
            << "\n";
        // 2. 최소한의 안정성 체크만 수행
        // 여기서는 절대 closing 하지 않는다
        if (header.bodySize > IOCP_BUFFER_SIZE)
        {
            std::cout
                << "[PARSER WARN] invalid bodySize "
                << "session=" << session->sessionId
                << " bodySize=" << header.bodySize
                << "\n";

            // 버퍼 초기화 (sync 깨졌다고 가정)
            used = 0;
            return;
        }

        int totalSize = (int)sizeof(PACKET_HEADER) + header.bodySize;

        // 3. 패킷 전체 도착 여부 확인
        if (used < totalSize)
            return;

        // 4. 패킷 처리 (보안 판단은 PacketManager에서)
        const char* body = buf + sizeof(PACKET_HEADER);

        PacketManager::HandlePacket(
            session,
            header.packetId,
            header.token,
            body,
            header.bodySize
        );

        // 5. 처리한 데이터 제거
        memmove(buf, buf + totalSize, used - totalSize);
        used -= totalSize;
    }
}
