#include "IOCP_Recv.h"
#include "PacketParser.h"
#include "PacketManager.h"
#include <algorithm>
#include "IOCP_Defines.h"
#include <iostream>

// 비동기 WSARecv를 등록하여 세션의 다음 수신을 대기시키는 함수
void IOCP_Recv::PostRecv(Session* session)
{
    if (!session)
    {
        std::cout << "[RECV][POST] session nullptr\n";
        return;
    }

    if (session->closing)
    {
        PacketManager::HandleSessionDisconnect(session);
        shutdown(session->sock, SD_BOTH);
        std::cout
            << "[RECV][POST] 중단됨 (closing=true) "
            << "sessionId=" << session->sessionId
            << "\n";
        return;
    }

    OverlappedEx* ov = ovPool->Acquire();
    ZeroMemory((OVERLAPPED*)ov, sizeof(OVERLAPPED));

    ov->op = OverlappedEx::OP_RECV;
    ov->session = session;

    ov->bufferPtr = bufPool->Acquire();
    ov->wsaBuf.buf = ov->bufferPtr;
    ov->wsaBuf.len = IOCP_BUFFER_SIZE;

    DWORD flags = 0;
    DWORD recvBytes = 0;

    int ret = WSARecv(
        session->sock,
        &ov->wsaBuf,
        1,
        &recvBytes,
        &flags,
        (LPWSAOVERLAPPED)ov,
        nullptr
    );

    if (ret == SOCKET_ERROR)
    {
        int err = WSAGetLastError();

        if (err != WSA_IO_PENDING)
        {
            std::cout
                << "[RECV][POST][ERROR] WSARecv 실패 "
                << "err=" << err
                << " sessionId=" << session->sessionId
                << "\n";

            session->closing = true;
            PacketManager::HandleSessionDisconnect(session);
            shutdown(session->sock, SD_BOTH);

            bufPool->Release(ov->bufferPtr);
            ovPool->Release(ov);
        }
    }
}

// 수신 완료된 데이터를 세션 버퍼에 누적하고 패킷 파싱 후 다음 수신을 재등록하는 함수
void IOCP_Recv::RecvCompletion(Session* session, OverlappedEx* ov, DWORD bytes)
{

    std::lock_guard<std::recursive_mutex> lock(session->stateMtx);
    if (bytes == 0 || bytes > IOCP_BUFFER_SIZE) session->closing = true;
    size_t offset = 0;
    while (!session->closing && offset < bytes)
    {
        if (session->recvUsed < 0 || session->recvUsed >= IOCP_BUFFER_SIZE)
        { session->closing = true; break; }
        const auto count = (std::min)(size_t(IOCP_BUFFER_SIZE - session->recvUsed),
            size_t(bytes) - offset);
        memcpy(session->recvBuffer + session->recvUsed, ov->bufferPtr + offset, count);
        session->recvUsed += static_cast<int>(count);
        offset += count;
        PacketParser::ProcessBuffer(session);
    }
    bufPool->Release(ov->bufferPtr);
    ovPool->Release(ov);
    if (session->closing)
    {
        PacketManager::HandleSessionDisconnect(session);
        shutdown(session->sock, SD_BOTH);
    }
    else PostRecv(session);
}
