#pragma once
#include <vector>
#include <mutex>
#include "Session.h"

class SessionPool
{
public:
    SessionPool(int capacity = 2000)
    {
        pool.reserve(capacity);
        for (int i = 0; i < capacity; i++)
            pool.push_back(new Session());
    }

    ~SessionPool()
    {
        for (Session* s : pool)
            delete s;
    }

    Session* Acquire()
    {
        std::lock_guard<std::mutex> lock(mtx);

        if (!pool.empty())
        {
            Session* s = pool.back();
            pool.pop_back();
            s->Reset();
            return s;
        }

        Session* s = new Session();
        s->Reset();
        return s;
    }

    void Release(Session* session)
    {
        session->Reset();
        std::lock_guard<std::mutex> lock(mtx);
        pool.push_back(session);
    }

private:
    std::vector<Session*> pool;
    std::mutex mtx;
};
