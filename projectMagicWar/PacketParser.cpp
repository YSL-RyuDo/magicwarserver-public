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
        // 2. 헤더를 포함한 전체 프레임이 누적 버퍼에 들어가야 한다.
        if (header.bodySize > IOCP_BUFFER_SIZE - sizeof(PACKET_HEADER))
        {
            std::cout
                << "[PARSER WARN] invalid bodySize "
                << "session=" << session->sessionId
                << " bodySize=" << header.bodySize
                << "\n";

            // 잘못된 길이 뒤에서 임의로 재동기화하지 않고 연결을 종료한다.
            session->closing = true;
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
        if (session->closing) return;
    }
}
