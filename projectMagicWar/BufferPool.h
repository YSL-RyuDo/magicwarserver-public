#pragma once
#include <vector>
#include <mutex>

#include "IOCP_Defines.h"

class BufferPool
{
public:
    char* Acquire()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!pool.empty())
        {
            char* buf = pool.back();
            pool.pop_back();
            return buf;
        }
        return new char[IOCP_BUFFER_SIZE];
    }

    void Release(char* buf)
    {
        std::lock_guard<std::mutex> lock(mtx);
        pool.push_back(buf);
    }

private:
    std::vector<char*> pool;
    std::mutex mtx;
};
