#include "MatchManager.h"
#include "PacketManager.h"
#include <algorithm>
#include <iostream>
#include <cstring>

MatchManager::MatchManager()
{
}

//----------------------------------------------
//  매칭 요청
//----------------------------------------------
void MatchManager::RequestMatch(Session* session)
{
    std::lock_guard<std::mutex> lock(queueMtx);

    // 이미 대기열에 있는지 확인
    std::queue<Session*> temp;
    bool alreadyInQueue = false;

    while (!matchQueue.empty())
    {
        Session* s = matchQueue.front();
        matchQueue.pop();

        if (s == session)
            alreadyInQueue = true;

        temp.push(s);
    }
    matchQueue = temp;

    // 이미 존재하는 유저 → "AlreadyWaiting"
    PKT_MATCH_STATE pkt{};
    if (alreadyInQueue)
    {
        memcpy(pkt.msg, "AlreadyWaiting", 15);
        //PacketManager::SendPacket(session, PKT_MATCH_WAIT, &pkt, sizeof(pkt));
        return;
    }

    // 대기열에 새로 추가
    matchQueue.push(session);

    memcpy(pkt.msg, "Wait", 4);
    //PacketManager::SendPacket(session, PKT_MATCH_WAIT, &pkt, sizeof(pkt));

    TryMatch();
}

//----------------------------------------------
//  매칭 취소
//----------------------------------------------
void MatchManager::CancelMatch(Session* session)
{
    std::lock_guard<std::mutex> lock(queueMtx);

    std::queue<Session*> temp;
    bool found = false;

    while (!matchQueue.empty())
    {
        Session* s = matchQueue.front();
        matchQueue.pop();

        if (s == session)
        {
            found = true;
            continue;
        }
        temp.push(s);
    }
    matchQueue = temp;

    PKT_MATCH_STATE pkt{};

    if (found)
        memcpy(pkt.msg, "CancelOK", 8);
    else
        memcpy(pkt.msg, "NotInQueue", 11);

    //PacketManager::SendPacket(session, PKT_MATCH_CANCEL_OK, &pkt, sizeof(pkt));
}

//----------------------------------------------
//  매칭 처리
//----------------------------------------------
void MatchManager::TryMatch()
{
    std::lock_guard<std::mutex> lock(queueMtx);

    while (matchQueue.size() >= 2)
    {
        Session* a = matchQueue.front();
        matchQueue.pop();

        Session* b = matchQueue.front();
        matchQueue.pop();

        if (!a->authenticated || !b->authenticated)
            continue;

        // A가 받을 패킷 (상대 b 닉네임)
        PKT_MATCH_READY_ACK pktA{};
        memcpy(pktA.opponent, b->nickname.c_str(), b->nickname.size());

        // B가 받을 패킷 (상대 a 닉네임)
        PKT_MATCH_READY_ACK pktB{};
        memcpy(pktB.opponent, a->nickname.c_str(), a->nickname.size());

        //PacketManager::SendPacket(a, PKT_MATCH_READY, &pktA, sizeof(pktA));
        //PacketManager::SendPacket(b, PKT_MATCH_READY, &pktB, sizeof(pktB));

        std::cout << "[MATCH] " << a->nickname << " vs " << b->nickname << "\n";
    }
}
