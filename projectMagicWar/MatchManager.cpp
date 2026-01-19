#include "MatchManager.h"
#include "PacketManager.h"
#include "DBWorkerInstance.h"
#include <algorithm>
#include <iostream>
#include <cstring>

MatchManager::MatchManager()
{
}

//  매칭 요청
void MatchManager::RequestMatch(Session* session)
{
    std::lock_guard<std::mutex> lock(queueMtx);

    matchQueue.push(session);

    PKT_MATCH_RESPONSE_STRUCT pkt{};
    pkt.result = 0; // WAIT
    pkt.roomId = 0;

    PacketManager::SendPacket(
        session,
        PKT_MATCH_RESPONSE,
        &pkt,
        sizeof(pkt)
    );

    TryMatch();
}


//  매칭 취소
void MatchManager::CancelMatch(Session* session)
{
    std::lock_guard<std::mutex> lock(queueMtx);

    std::queue<Session*> temp;
    while (!matchQueue.empty())
    {
        Session* s = matchQueue.front(); matchQueue.pop();
        if (s != session)
            temp.push(s);
    }
    matchQueue = temp;

    PKT_MATCH_RESPONSE_STRUCT pkt{};
    pkt.result = 2; // FAIL / CANCEL

    PacketManager::SendPacket(
        session,
        PKT_MATCH_RESPONSE,
        &pkt,
        sizeof(pkt)
    );
}

//  매칭 처리
void MatchManager::TryMatch()
{
    if (matchQueue.size() < 2)
        return;

    std::queue<Session*> remainQueue;

    while (!matchQueue.empty())
    {
        Session* a = matchQueue.front();
        matchQueue.pop();

        if (!a || !a->authenticated || a->inBattle)
            continue;

        bool matched = false;

        std::queue<Session*> tempQueue;

        while (!matchQueue.empty())
        {
            Session* b = matchQueue.front();
            matchQueue.pop();

            if (!b || !b->authenticated || b->inBattle)
            {
                tempQueue.push(b);
                continue;
            }

            auto now = std::chrono::steady_clock::now();
            int waitA = std::chrono::duration_cast<std::chrono::seconds>(
                now - a->queueEnterTime).count();
            int waitB = std::chrono::duration_cast<std::chrono::seconds>(
                now - b->queueEnterTime).count();

            int waitTime = (waitA < waitB) ? waitA : waitB;
            int allowedMMR = GetAllowedMMRRange(waitTime);

            if (abs(a->mmr - b->mmr) > allowedMMR)
            {
                tempQueue.push(b);
                continue;
            }

            if (!IsRankCompatible(a->rank, b->rank, waitTime))
            {
                tempQueue.push(b);
                continue;
            }

            // 매칭 성사 
            StartBattle(a, b);
            matched = true;
            break;
        }

        // tempQueue 복구
        while (!tempQueue.empty())
        {
            matchQueue.push(tempQueue.front());
            tempQueue.pop();
        }

        if (!matched)
            remainQueue.push(a);
    }

    matchQueue = remainQueue;
}

void MatchManager::StartBattle(Session* a, Session* b)
{
    static uint32_t roomIdGen = 1;
    uint32_t roomId = roomIdGen++;

    a->roomId = roomId;
    b->roomId = roomId;

    a->inBattle = true;
    b->inBattle = true;

    a->battleOpponent = b;
    b->battleOpponent = a;

    a->currentHp = a->maxHp = 100;
    b->currentHp = b->maxHp = 100;

    PKT_MATCH_RESPONSE_STRUCT pkt{};
    pkt.result = 1;
    pkt.roomId = roomId;

    strncpy_s(pkt.userA, a->userId.c_str(), 31);
    strncpy_s(pkt.userB, b->userId.c_str(), 31);

    PacketManager::SendPacket(a, PKT_MATCH_RESPONSE, &pkt, sizeof(pkt));
    PacketManager::SendPacket(b, PKT_MATCH_RESPONSE, &pkt, sizeof(pkt));

    std::cout << "[MATCH] room=" << roomId
        << " " << a->userId
        << " vs " << b->userId << std::endl;
}


//void MatchManager::ProcessResult(Session* winner, Session* loser)
//{
//    if (!winner || !loser)
//        return;
//
//    // ------------------------
//    // 랭크 데이터 로드
//    // ------------------------
//    UserRankData winRank{};
//    UserRankData loseRank{};
//
//    if (!UserManager::LoadUserRank(winner->userId, winRank)) return;
//    if (!UserManager::LoadUserRank(loser->userId, loseRank)) return;
//
//    // ------------------------
//    // 랭크 차이 계산
//    // ------------------------
//    int rankDiff = winRank.rank - loseRank.rank;
//    int baseDelta = CalculateBaseDelta(abs(rankDiff));
//
//    int winDelta = baseDelta;
//    int loseDelta = -baseDelta;
//
//    // 낮은 랭크가 이긴 경우 → 보너스
//    if (rankDiff < 0)
//        winDelta = static_cast<int>(winDelta * 1.5f);
//
//    // 높은 랭크가 진 경우 → 패널티 증가
//    if (rankDiff > 0)
//        loseDelta = static_cast<int>(loseDelta * 1.5f);
//
//    // ------------------------
//    // 점수 반영
//    // ------------------------
//    winRank.mmr += winDelta;
//    winRank.rankScore += winDelta;
//
//    loseRank.mmr += loseDelta;
//    loseRank.rankScore += loseDelta;
//
//    // 음수 방지
//    if (loseRank.rankScore < 0) loseRank.rankScore = 0;
//    if (loseRank.mmr < 0) loseRank.mmr = 0;
//
//    // ------------------------
//    // 랭크 재계산
//    // ------------------------
//    winRank.rank = CalculateRankFromScore(winRank.rankScore);
//    loseRank.rank = CalculateRankFromScore(loseRank.rankScore);
//
//    // ------------------------
//    // 랭크 저장
//    // ------------------------
//    UserManager::SaveUserRank(winner->userId, winRank);
//    UserManager::SaveUserRank(loser->userId, loseRank);
//
//    // ------------------------
//    // UserInfo (승/패/EXP)
//    // ------------------------
//    UserInfoData winInfo{};
//    UserInfoData loseInfo{};
//
//    UserManager::LoadUserInfo(winner->userId, winInfo);
//    UserManager::LoadUserInfo(loser->userId, loseInfo);
//
//    // 승패 기록
//    winInfo.win += 1;
//    loseInfo.lose += 1;
//
//    // 경험치 계산
//    int winExp = 50 + (loseRank.rank * 10);
//    int loseExp = 20 + (winRank.rank * 5);
//
//    winInfo.exp += winExp;
//    loseInfo.exp += loseExp;
//
//    // 레벨업 처리 (레벨 * 100 기준)
//    while (winInfo.exp >= winInfo.level * 100)
//    {
//        winInfo.exp -= winInfo.level * 100;
//        winInfo.level += 1;
//    }
//
//    while (loseInfo.exp >= loseInfo.level * 100)
//    {
//        loseInfo.exp -= loseInfo.level * 100;
//        loseInfo.level += 1;
//    }
//
//    // 저장
//    UserManager::SaveUserRank(winner->userId, winRank);
//    UserManager::SaveUserRank(loser->userId, loseRank);
//
//    std::cout << "[MATCH RESULT] "
//        << winner->userId << " WIN (+"
//        << winDelta << " MMR) | "
//        << loser->userId << " LOSE ("
//        << loseDelta << " MMR)\n";
//}

void MatchManager::ProcessResultById(
    const std::string& winnerId,
    const std::string& loserId)
{
    // DBWorker에는 "DB 저장"만 맡김
    g_dbWorker->Enqueue([winnerId, loserId]() {

        UserRankData winRank{}, loseRank{};
        UserInfoData winInfo{}, loseInfo{};

        if (!UserManager::LoadUserRank(winnerId, winRank)) return;
        if (!UserManager::LoadUserRank(loserId, loseRank)) return;

        // 계산
        int rankDiff = winRank.rank - loseRank.rank;
        int delta = CalculateBaseDelta(abs(rankDiff));

        winRank.mmr += delta;
        loseRank.mmr -= delta;

        UserManager::SaveUserRank(winnerId, winRank);
        UserManager::SaveUserRank(loserId, loseRank);

        UserManager::LoadUserInfo(winnerId, winInfo);
        UserManager::LoadUserInfo(loserId, loseInfo);

        winInfo.win++;
        loseInfo.lose++;

        UserManager::SaveUserInfo(winnerId, winInfo);
        UserManager::SaveUserInfo(loserId, loseInfo);
        });
}


int MatchManager::CalculateBaseDelta(int rankDiff)
{
    // 기본 점수
    if (rankDiff >= 3) return 40;
    if (rankDiff == 2) return 30;
    if (rankDiff == 1) return 25;
    return 20;
}

int MatchManager::CalculateRankFromScore(int rankScore)
{
    if (rankScore < 1000) return 0;        // Bronze
    if (rankScore < 1500) return 1;        // Silver
    if (rankScore < 2000) return 2;        // Gold
    if (rankScore < 2500) return 3;        // Platinum
    if (rankScore < 3000) return 4;        // Diamond
    if (rankScore < 3500) return 5;        // Master
    return 6;                              // Grandmaster 이상
}

int MatchManager::GetAllowedMMRRange(int waitSeconds)
{
    if (waitSeconds < 5) return 50;
    if (waitSeconds < 10) return 100;
    if (waitSeconds < 20) return 200;
    return 500;
}

bool MatchManager::IsRankCompatible(int rankA, int rankB, int waitSeconds)
{
    int allowedRankDiff = (waitSeconds < 10) ? 1 : 2;
    return abs(rankA - rankB) <= allowedRankDiff;
}