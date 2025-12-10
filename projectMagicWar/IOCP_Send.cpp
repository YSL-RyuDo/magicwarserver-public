#include "IOCP_Send.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")


void IOCP_Send::EnqueueSend(Session* session, const char* data, size_t len)
{
    if (session->closing) return;

    {
        std::lock_guard<std::mutex> lock(session->sendMtx);
        session->sendQueue.emplace(data, data + len);
    }

    bool expected = false;
    if (session->sending.compare_exchange_strong(expected, true))
        PostSend(session);
}

void IOCP_Send::PostSend(Session* session)
{
    if (session->closing) return;

    std::vector<char> msg;

    {
        std::lock_guard<std::mutex> lock(session->sendMtx);

        if (session->sendQueue.empty())
        {
            session->sending = false;
            return;
        }

        msg = std::move(session->sendQueue.front());
        session->sendQueue.pop();
    }

    OverlappedEx* ov = ovPool->Acquire();
    ZeroMemory((OVERLAPPED*)ov, sizeof(OVERLAPPED));

    char* buf = bufPool->Acquire();

    ov->op = OverlappedEx::OP_SEND;
    ov->session = session;
    ov->bufferPtr = buf;

    memcpy(buf, msg.data(), msg.size());
    ov->wsaBuf.buf = buf;
    ov->wsaBuf.len = (ULONG)msg.size();

    DWORD flags = 0, sentBytes = 0;
    int ret = WSASend(
        session->sock,
        &ov->wsaBuf,
        1,
        &sentBytes,
        flags,
        (LPWSAOVERLAPPED)ov,
        nullptr
    );

    if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
    {
        session->closing = true;
        bufPool->Release(buf);
        ovPool->Release(ov);
    }
}

void IOCP_Send::SendCompletion(Session* session, OverlappedEx* ov)
{
    bufPool->Release(ov->bufferPtr);
    ovPool->Release(ov);

    PostSend(session);
}
