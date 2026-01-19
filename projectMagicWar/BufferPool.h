#pragma once
#include <vector>
#include <mutex>

#include "IOCP_Defines.h"

using namespace std;
// IOCP 서버에서 고정 크기 버퍼를 재사용하기 위한 간단한 버퍼 풀 클래스입니다.
class BufferPool
{
public:
    // 사용 가능한 버퍼를 하나 획득하거나 없으면 새로 할당합니다.
    char* Acquire()
    {
        lock_guard<mutex> lock(mtx);
        if (!pool.empty())
        {
            char* buf = pool.back();
            pool.pop_back();
            return buf;
        }
        return new char[IOCP_BUFFER_SIZE];
    }

    // 사용이 끝난 버퍼를 풀에 반환하여 재사용합니다.
    void Release(char* buf)
    {
        lock_guard<mutex> lock(mtx);
        pool.push_back(buf);
    }

private:
    // 재사용 가능한 버퍼 포인터들을 저장하는 컨테이너입니다.
    vector<char*> pool;

    // 멀티스레드 환경에서 풀 접근을 보호하기 위한 뮤텍스입니다.
    mutex mtx;
};
