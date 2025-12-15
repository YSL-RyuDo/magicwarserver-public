#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>

struct CardInfo
{
    int id;
    std::string name;
    int attack;
    int cooldown;
    int cost;
    std::string effectType;
    float speed;
    float radius;
    float duration;
    int amount;
};

struct RankInfo
{
    int rank;
    std::string name;
    int minScore;
    int maxScore;
};

struct UserInfoData
{
    int level;
    int exp;
    int win;
    int lose;
};

struct UserRankData
{
    int mmr;
    int rank;
    int rankScore;
};

struct UserProfileData
{
    UserInfoData info;
    UserRankData rank;
};

class UserManager
{
public:
    // ===== Account =====
    static bool AddUser(
        const std::string& id,
        const std::string& hash,
        const std::string& salt,
        const std::string& nick);

    static bool CheckLogin(
        const std::string& id,
        const std::string& pw,
        std::string& outNick);

    static bool IsIDExist(const std::string& id);
    static bool IsNickExist(const std::string& nick);

    // ===== Login state =====
    static bool TryLoginUser(const std::string& id);
    static void LogoutUser(const std::string& id);

    // ===== Load / Save =====
    static bool LoadUserInfo(const std::string& id, UserInfoData& out);
    static bool SaveUserInfo(const std::string& id, const UserInfoData& data);

    static bool LoadUserRank(const std::string& id, UserRankData& out);
    static bool SaveUserRank(const std::string& id, const UserRankData& data);

    static bool LoadUserMagic(const std::string& id, std::vector<int>& out);
    static bool ReplaceUserMagic(const std::string& id, int oldCard, int newCard);

    static bool LoadUserProfile(const std::string& id, UserProfileData& out);

    // ===== Static tables =====
    static bool LoadCardTable();
    static bool LoadRankTable();

    static const CardInfo* GetCardInfo(int id);
    static const RankInfo* GetRankInfo(int rank);

    // ===== Validation =====
    static bool ValidateUser(const std::string& id);

private:
    static std::mutex s_mtx;
    static std::unordered_set<std::string> s_loggedIn;
    static std::unordered_map<int, CardInfo> s_cards;
    static std::unordered_map<int, RankInfo> s_ranks;
};
