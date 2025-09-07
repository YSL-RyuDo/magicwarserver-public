#define NOMINMAX
#include "MatchManager.h"
#include <cstdlib>   // rand()
#include <algorithm> // remove_if

void MatchManager::Init(std::unordered_map<std::string, Client>* clientsMapPtr)
{
    clientsMap = clientsMapPtr;
}

// CSV에서 UserStat 읽기
UserStat MatchManager::LoadUserStat(const std::string& userId)
{
    std::ifstream file("UserStats.csv");
    if (!file.is_open())
    {
        std::cerr << "[매칭] UserStats.csv 파일 열기 실패" << std::endl;
        return {};
    }

    std::string line;
    std::getline(file, line); // 헤더 스킵

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string id, mmrStr, rankStr, scoreStr;
        std::getline(ss, id, ',');
        std::getline(ss, mmrStr, ',');
        std::getline(ss, rankStr, ',');
        std::getline(ss, scoreStr, ',');

        if (id == userId)
        {
            UserStat stat;
            stat.id = id;
            stat.mmr = std::stoi(mmrStr);
            stat.rank = std::stoi(rankStr);
            stat.rankScore = std::stoi(scoreStr);
            return stat;
        }
    }

    std::cerr << "[매칭] UserID 없음: " << userId << std::endl;
    return {};
}

void MatchManager::SendOk(const MatchRequest& a, const MatchRequest& b, const std::string& roomId)
{
    if (!clientsMap) return;

    std::string msg = "OK," + roomId + "," + a.userId + "," + b.userId;

    auto ita = clientsMap->find(a.userId);
    if (ita != clientsMap->end()) {
        PacketManager::sendPacket(ita->second.socket, PacketType::ResponseMatching, msg);
    }

    auto itb = clientsMap->find(b.userId);
    if (itb != clientsMap->end()) {
        PacketManager::sendPacket(itb->second.socket, PacketType::ResponseMatching, msg);
    }

    std::cout << "[매칭 완료 전송] Room=" << roomId
        << " A=" << a.userId
        << " B=" << b.userId
        << std::endl;
}

void MatchManager::SendCancel(const std::string& userId)
{
    if (!clientsMap) return;

    std::string msg = "CANCEL";

    auto it = clientsMap->find(userId);
    if (it != clientsMap->end()) {
        PacketManager::sendPacket(it->second.socket, PacketType::ResponseMatching, msg);
        std::cout << "[매칭 취소 전송] userId=" << userId << std::endl;
    }
    else {
        std::cout << "[매칭 취소 실패] userId=" << userId << " (클라이언트 없음)" << std::endl;
    }
}

// 매칭 시작 처리
void MatchManager::StartMatch(const MatchRequest& a, const MatchRequest& b)
{
    std::string roomId = "Room" + std::to_string(rand() % 10000);
    SendOk(a, b, roomId);

    std::cout << "[매칭 완료] " << a.userId << " vs " << b.userId
        << " → " << roomId << std::endl;
}

// 매칭 요청 처리
// 매칭 요청 처리
void MatchManager::HandleRequestMatching(const std::string& userId)
{
    std::cout << "[매칭 요청] " << userId << std::endl;

    UserStat stat = LoadUserStat(userId);
    if (stat.id.empty())
    {
        std::cerr << "[매칭 실패] UserStat 불러오기 실패" << std::endl;
        return;
    }

    {
        // 대기열 추가 구간만 잠금
        std::lock_guard<std::mutex> lock(waitingMutex);

        auto it = std::find_if(waitingPlayers.begin(), waitingPlayers.end(),
            [&](const MatchRequest& req) { return req.userId == userId; });

        if (it != waitingPlayers.end()) {
            std::cout << "[매칭 요청 무시] " << userId << " 이미 대기열에 있음" << std::endl;
            return;
        }

        MatchRequest current{ stat.id, stat.mmr, stat.rank, stat.rankScore,
                              std::chrono::steady_clock::now() };
        waitingPlayers.push_back(current);

        std::cout << "[매칭 대기] " << userId << " 대기열 등록" << std::endl;
    }

    //  잠금이 풀린 상태에서 호출해야 deadlock 안 생김
    Update();
}


// 매칭 취소 처리
void MatchManager::HandleCancelMatch(const std::string& userId)
{
    std::lock_guard<std::mutex> lock(waitingMutex);

    auto it = std::remove_if(waitingPlayers.begin(), waitingPlayers.end(),
        [&](const MatchRequest& req) { return req.userId == userId; });

    if (it != waitingPlayers.end())
    {
        waitingPlayers.erase(it, waitingPlayers.end());
        std::cout << "[매칭 취소] " << userId << " 대기열에서 제거됨" << std::endl;
        SendCancel(userId);
    }
    else
    {
        std::cout << "[매칭 취소 실패] " << userId << " 는 대기열에 없음" << std::endl;
    }
}

// 주기적으로 호출해서 자동 매칭
void MatchManager::Update()
{
    std::lock_guard<std::mutex> lock(waitingMutex);

    if (waitingPlayers.size() < 2) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    bool matched = false;

    for (size_t i = 0; i < waitingPlayers.size() && !matched; i++)
    {
        for (size_t j = i + 1; j < waitingPlayers.size() && !matched; j++)
        {
            auto& a = waitingPlayers[i];
            auto& b = waitingPlayers[j];

            int rankDiff = abs(a.rank - b.rank);
            auto waitTimeA = now - a.requestTime;
            auto waitTimeB = now - b.requestTime;
            auto maxWait = std::max(waitTimeA, waitTimeB);

            std::cout << "[매칭 검사] " << a.userId << " vs " << b.userId
                << " | rankDiff=" << rankDiff
                << " | waitA=" << std::chrono::duration_cast<std::chrono::seconds>(waitTimeA).count() << "s"
                << " | waitB=" << std::chrono::duration_cast<std::chrono::seconds>(waitTimeB).count() << "s"
                << " | maxWait=" << std::chrono::duration_cast<std::chrono::seconds>(maxWait).count() << "s"
                << std::endl;

            if (rankDiff <= ALLOWED_DIFF ||
                maxWait > std::chrono::seconds(FORCE_MATCH_SEC))
            {
                std::cout << "[매칭 조건 충족] → 매칭 시작" << std::endl;
                StartMatch(a, b);

                if (j > i) {
                    waitingPlayers.erase(waitingPlayers.begin() + j);
                    waitingPlayers.erase(waitingPlayers.begin() + i);
                }
                else {
                    waitingPlayers.erase(waitingPlayers.begin() + i);
                    waitingPlayers.erase(waitingPlayers.begin() + j);
                }
                matched = true;
            }
            else
            {
                std::cout << "[매칭 조건 불충족] → 대기 유지" << std::endl;
            }
        }
    }
}
