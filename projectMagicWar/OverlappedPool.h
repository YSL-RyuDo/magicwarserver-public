#pragma once
#include <vector>
#include <mutex>
#include "OverlappedEx.h"

class OverlappedPool
{
public:
    OverlappedEx* Acquire()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!pool.empty())
        {
            OverlappedEx* ov = pool.back();
            pool.pop_back();
            return ov;
        }
        return new OverlappedEx();
    }

    void Release(OverlappedEx* ov)
    {
        ZeroMemory((OVERLAPPED*)ov, sizeof(OVERLAPPED));

        ov->op = OverlappedEx::OP_ACCEPT;
        ov->session = nullptr;
        ov->acceptSock = INVALID_SOCKET;

        ov->bufferPtr = nullptr;
        ov->wsaBuf.buf = nullptr;
        ov->wsaBuf.len = 0;

        std::lock_guard<std::mutex> lock(mtx);
        pool.push_back(ov);
    }

private:
    std::vector<OverlappedEx*> pool;
    std::mutex mtx;
};
