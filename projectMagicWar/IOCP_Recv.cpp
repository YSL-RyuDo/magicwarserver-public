#include "IOCP_Recv.h"
#include "PacketParser.h"
#include "IOCP_Defines.h"
#include <iostream>

void IOCP_Recv::PostRecv(Session* session)
{
    if (!session)
    {
        std::cout << "[RECV][POST] session nullptr\n";
        return;
    }

    std::cout
        << "[RECV][POST] 진입 "
        << "sessionId=" << session->sessionId
        << " closing=" << session->closing
        << " sock=" << session->sock
        << "\n";

    if (session->closing)
    {
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

            bufPool->Release(ov->bufferPtr);
            ovPool->Release(ov);
        }
        else
        {
            std::cout
                << "[RECV][POST] WSARecv 등록됨 (IO_PENDING) "
                << "sessionId=" << session->sessionId
                << "\n";
        }
    }
    else
    {
        std::cout
            << "[RECV][POST] WSARecv 즉시 완료 "
            << "recvBytes=" << recvBytes
            << " sessionId=" << session->sessionId
            << "\n";
    }
}


void IOCP_Recv::RecvCompletion(Session* session, OverlappedEx* ov, DWORD bytes)
{
    std::cout
        << "[RECV] RecvCompletion 진입 "
        << "bytes=" << bytes
        << " closing=" << session->closing
        << " recvUsed=" << session->recvUsed
        << "\n";

    if (bytes == 0)
    {
        std::cout
            << "[RECV] bytes=0 → 연결 종료 처리 "
            << "session=" << session->sessionId
            << "\n";
        session->closing = true;
        bufPool->Release(ov->bufferPtr);
        ovPool->Release(ov);
        return;
    }

    memcpy(session->recvBuffer + session->recvUsed, ov->bufferPtr, bytes);
    session->recvUsed += bytes;

    PacketParser::ProcessBuffer(session);

    bufPool->Release(ov->bufferPtr);
    ovPool->Release(ov);

    PostRecv(session);
}
