#pragma once
#include <winsock2.h>
#include <windows.h>

struct OverlappedEx : public OVERLAPPED
{
    enum OperationType
    {
        OP_ACCEPT,
        OP_RECV,
        OP_SEND
    };

    OperationType op;
    void* session;

    WSABUF wsaBuf;

    char* bufferPtr;

    SOCKET acceptSock;

    OverlappedEx()
    {
        ZeroMemory((OVERLAPPED*)this, sizeof(OVERLAPPED));

        op = OP_ACCEPT;
        session = nullptr;
        bufferPtr = nullptr;
        acceptSock = INVALID_SOCKET;

        wsaBuf.buf = nullptr;
        wsaBuf.len = 0;
    }
};
