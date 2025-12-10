#pragma once
#include <winsock2.h>
#include <vector>
#include <mutex>
#include "Session.h"
#include "OverlappedEx.h"
#include "OverlappedPool.h"
#include "BufferPool.h"

class IOCP_Send
{
public:
    IOCP_Send(OverlappedPool* o, BufferPool* b)
        : ovPool(o), bufPool(b) {
    }

    void EnqueueSend(Session* session, const char* data, size_t len);
    void PostSend(Session* session);
    void SendCompletion(Session* session, OverlappedEx* ov);

private:
    OverlappedPool* ovPool;
    BufferPool* bufPool;
};
