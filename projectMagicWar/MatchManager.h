#pragma once
#include <string>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <algorithm>

#include "PacketType.h"
#include "Client.h"
#include "PacketManager.h"   // PacketManager 사용

struct UserStat {
    std::string id;
    int mmr;
    int rank;
    int rankScore;
};

struct MatchRequest {
    std::string userId;
    int mmr;
    int rank;
    int rankScore;
    std::chrono::steady_clock::time_point requestTime;
};

class MatchManager
{
public:
    // PacketManager 포인터 연결용
    void Init(std::unordered_map<std::string, Client>* clientsMapPtr);

    void HandleRequestMatching(const std::string& userId);
    void HandleCancelMatch(const std::string& userId);
    void Update(); // 주기적으로 호출해서 자동 매칭

private:
    std::vector<MatchRequest> waitingPlayers;
    std::mutex waitingMutex;

    std::unordered_map<std::string, Client>* clientsMap = nullptr;

    const int ALLOWED_DIFF = 2;         // 허용 티어 차이
    const int FORCE_MATCH_SEC = 10;     // 강제 매칭 시간 (초)

    UserStat LoadUserStat(const std::string& userId);
    void StartMatch(const MatchRequest& a, const MatchRequest& b);
    void SendOk(const MatchRequest& a, const MatchRequest& b, const std::string& roomId);
    void SendCancel(const std::string& userId);
};
