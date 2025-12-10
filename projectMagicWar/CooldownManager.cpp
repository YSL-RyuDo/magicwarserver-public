#include "CooldownManager.h"
#include "MagicManager.h"

std::unordered_map<uint64_t, std::unordered_map<int, long long>> CooldownManager::s_cool;

bool CooldownManager::IsReady(Session* session, int magicId)
{
    MagicInfo info;
    if (!MagicManager::GetMagicInfo(magicId, info))
        return false;

    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    auto& userMap = s_cool[session->sessionId];

    if (userMap.count(magicId) == 0)
        return true;

    long long last = userMap[magicId];

    return (now - last) >= (long long)(info.cooldown * 1000);
}

void CooldownManager::Start(Session* session, int magicId)
{
    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    s_cool[session->sessionId][magicId] = now;
}
