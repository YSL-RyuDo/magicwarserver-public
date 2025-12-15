#pragma once
#include <winsock2.h>
#include <string>
#include <queue>
#include <atomic>
#include <mutex>

#include "IOCP_Defines.h"

class Session
{
public:
    Session() { Reset(); }

    ~Session()
    {
        if (sock != INVALID_SOCKET)
            closesocket(sock);
    }

    void Reset()
    {
        sock = INVALID_SOCKET;
        sessionId = 0;

        recvUsed = 0;
        ZeroMemory(recvBuffer, IOCP_BUFFER_SIZE);
        while (!sendQueue.empty())
            sendQueue.pop();

        sending = false;
        closing = false;

        authenticated = false;
        userId.clear();
        nickname.clear();

        hp = 100;
        inBattle = false;
        battleOpponent = nullptr;
    }

public:
    SOCKET sock;
    uint64_t sessionId;
    uint32_t roomId = 0;
    int currentHp = 100;
    int maxHp = 100;
    float posX = 0.f;
    float posY = 0.f;

    char recvBuffer[IOCP_BUFFER_SIZE];
    int recvUsed;

    std::queue<std::vector<char>> sendQueue;
    std::mutex sendMtx;
    std::atomic<bool> sending;

    bool authenticated;
    std::string userId;
    std::string nickname;

    std::atomic<bool> closing;

    int hp;
    bool inBattle;
    Session* battleOpponent;

    uint32_t token = 0;

    int mmr;
    int rank;
    int rankScore;
    std::chrono::steady_clock::time_point queueEnterTime;

    bool isSlowed = false;
    bool isStunned = false;

    float slowRatio = 1.0f;

    uint64_t slowEndTime = 0;
    uint64_t stunEndTime = 0;
};
