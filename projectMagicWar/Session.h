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
};
