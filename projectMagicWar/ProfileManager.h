#pragma once
#include <string>
#include <mutex>

struct ProfileData
{
    std::string id;
    int mmr;
    int rank;
    int rankScore;
    int level;
    int exp;
    int win;
    int lose;
};

class ProfileManager
{
public:
    ProfileManager(const std::string& statsPath,
        const std::string& recordPath);

    bool LoadProfile(const std::string& id, ProfileData& out);
    std::string BuildPayload(const ProfileData& p);

private:
    bool LoadStats(const std::string& id, ProfileData& out);
    bool LoadRecord(const std::string& id, ProfileData& out);

private:
    std::string statsPath_;
    std::string recordPath_;

    static std::mutex fileMutex_;
};
