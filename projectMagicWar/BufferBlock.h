#pragma once
#include <atomic>

struct BufferBlock
{
    char* data;      // 실제 데이터
    int   size;      // 실제 사용 크기
};
