#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

struct MagicInfo
{
    int id;
    std::string name;
    float cooldown;
    int damage;
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
