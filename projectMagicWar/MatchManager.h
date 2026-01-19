#pragma once
#include <queue>
#include <mutex>
#include <unordered_map>
#include "Session.h"
#include "UserManager.h"
class MatchManager
{
public:
    MatchManager();

    void RequestMatch(Session* session);
    void CancelMatch(Session* session);
    static void ProcessResultById(
        const std::string& winnerId,
        const std::string& loserId);

private:
    void TryMatch();
    void StartBattle(Session* a, Session* b);
    static int CalculateBaseDelta(int rankDiff);
    static int CalculateRankFromScore(int rankScore);
    static int GetAllowedMMRRange(int waitSeconds);
    bool IsRankCompatible(int rankA, int rankB, int waitSeconds);
    std::queue<Session*> matchQueue;
    std::mutex queueMtx;
};
