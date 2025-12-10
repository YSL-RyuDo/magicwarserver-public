#pragma once
#include <winsock2.h>
#include "Session.h"
#include "OverlappedEx.h"
#include "BufferPool.h"
#include "OverlappedPool.h"

class IOCP_Recv
{
public:
    IOCP_Recv(BufferPool* b, OverlappedPool* o)
        : bufPool(b), ovPool(o) {
    }

    void PostRecv(Session* session);
    void RecvCompletion(Session* session, OverlappedEx* ov, DWORD bytes);

private:
    BufferPool* bufPool;
    OverlappedPool* ovPool;
};
