#include "MatchManager.h"
#include "PacketManager.h"
#include <algorithm>
#include <iostream>

MatchManager::MatchManager()
{
}

void MatchManager::RequestMatch(Session* session)
{
    std::lock_guard<std::mutex> lock(queueMtx);

    // 이미 대기열에 있는지 검사
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

    if (alreadyInQueue)
    {
        PacketManager::SendPacket(session, PKT_MATCH_WAIT, "AlreadyWaiting");
        return;
    }

    // 대기열에 추가
    matchQueue.push(session);
    PacketManager::SendPacket(session, PKT_MATCH_WAIT, "Wait");

    TryMatch();
}

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

    if (found)
    {
        PacketManager::SendPacket(session, PKT_MATCH_CANCEL_OK, "CancelOK");
    }
    else
    {
        PacketManager::SendPacket(session, PKT_MATCH_CANCEL_OK, "NotInQueue");
    }
}

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

        std::string msgA = b->nickname;
        std::string msgB = a->nickname;

        // 매칭 성공 패킷 전송
        PacketManager::SendPacket(a, PKT_MATCH_READY, msgA);
        PacketManager::SendPacket(b, PKT_MATCH_READY, msgB);

        std::cout << "[MATCH] " << a->nickname << " vs " << b->nickname << "\n";
    }
}
