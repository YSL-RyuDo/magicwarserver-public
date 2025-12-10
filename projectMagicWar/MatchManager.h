#pragma once
#include <queue>
#include <mutex>
#include <unordered_map>
#include "Session.h"

class MatchManager
{
public:
    MatchManager();

    void RequestMatch(Session* session);
    void CancelMatch(Session* session);

private:
    void TryMatch();

    std::queue<Session*> matchQueue;
    std::mutex queueMtx;
};
