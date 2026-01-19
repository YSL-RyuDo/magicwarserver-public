#include "IOCP_Send.h"
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

// 송신 데이터를 세션의 송신 큐에 넣고 최초 송신이면 비동기 전송을 시작하는 함수
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

// 송신 큐에서 하나의 메시지를 꺼내 WSASend로 비동기 송신을 등록하는 함수
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

// 송신 완료 후 사용한 자원을 반환하고 다음 송신이 있으면 이어서 처리하는 함수
void IOCP_Send::SendCompletion(Session* session, OverlappedEx* ov)
{
    bufPool->Release(ov->bufferPtr);
    ovPool->Release(ov);

    PostSend(session);
}
