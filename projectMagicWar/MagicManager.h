#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

struct MagicInfo
{
    int id;                 // ID
    std::string name;       // Name

    int attack;             // Attack
    float cooldown;         // Cooldown
    int cost;               // Cost

    std::string effectType; // EffectType

    float speed;            // Speed (투사체 속도)
    float radius;           // Radius (범위)
    float duration;         // Duration (지속 시간)
    int amount;             // Amount (틱/회복량/횟수 등)
};


class MagicManager
{
public:
    static void Init(const std::string& magicInfoFile,
        const std::string& userMagicFile);

    static bool LoadUserMagic(const std::string& nick, std::vector<int>& outSlots);
    static bool ReplaceMagic(const std::string& nick, int slotIndex, int magicId);

    static bool GetMagicInfo(int magicId, MagicInfo& outInfo);

private:
    static std::mutex s_mtx;
    static std::string s_magicInfoFile;
    static std::string s_userMagicFile;

    static std::unordered_map<int, MagicInfo> s_magicTable;
};
