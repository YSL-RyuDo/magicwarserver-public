#include "IOCP_Recv.h"
#include "PacketParser.h"
#include "IOCP_Defines.h"
#include <iostream>

void IOCP_Recv::PostRecv(Session* session)
{
    if (session->closing) return;

    OverlappedEx* ov = ovPool->Acquire();
    ZeroMemory((OVERLAPPED*)ov, sizeof(OVERLAPPED));

    ov->op = OverlappedEx::OP_RECV;
    ov->session = session;

    ov->bufferPtr = bufPool->Acquire();
    ov->wsaBuf.buf = ov->bufferPtr;
    ov->wsaBuf.len = IOCP_BUFFER_SIZE;

    DWORD flags = 0, recvBytes = 0;

    int ret = WSARecv(
        session->sock,
        &ov->wsaBuf,
        1,
        &recvBytes,
        &flags,
        (LPWSAOVERLAPPED)ov,
        nullptr
    );

    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
    {
        session->closing = true;
        bufPool->Release(ov->bufferPtr);
        ovPool->Release(ov);
    }
}

void IOCP_Recv::RecvCompletion(Session* session, OverlappedEx* ov, DWORD bytes)
{
    if (bytes == 0)
    {
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
