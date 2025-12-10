#include "ProfileManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::mutex ProfileManager::fileMutex_;

ProfileManager::ProfileManager(
    const std::string& statsPath,
    const std::string& recordPath)
    : statsPath_(statsPath)
    , recordPath_(recordPath)
{
}

bool ProfileManager::LoadStats(const std::string& id, ProfileData& out)
{
    std::ifstream file(statsPath_);
    if (!file.is_open()) return false;

    std::string line;
    std::getline(file, line); // header skip

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string uid, mmr, rank, score;

        std::getline(ss, uid, ',');
        std::getline(ss, mmr, ',');
        std::getline(ss, rank, ',');
        std::getline(ss, score, ',');

        if (uid == id)
        {
            out.id = uid;
            out.mmr = stoi(mmr);
            out.rank = stoi(rank);
            out.rankScore = stoi(score);
            return true;
        }
    }
    return false;
}

bool ProfileManager::LoadRecord(const std::string& id, ProfileData& out)
{
    std::ifstream file(recordPath_);
    if (!file.is_open()) return false;

    std::string line;
    std::getline(file, line);

    while (std::getline(file, line))
    {
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string uid, level, exp, win, lose;

        std::getline(ss, uid, ',');
        std::getline(ss, level, ',');
        std::getline(ss, exp, ',');
        std::getline(ss, win, ',');
        std::getline(ss, lose, ',');

        if (uid == id)
        {
            out.level = stoi(level);
            out.exp = stoi(exp);
            out.win = stoi(win);
            out.lose = stoi(lose);
            return true;
        }
    }
    return false;
}

bool ProfileManager::LoadProfile(const std::string& id, ProfileData& out)
{
    std::lock_guard<std::mutex> lock(fileMutex_);

    if (!LoadStats(id, out)) return false;
    if (!LoadRecord(id, out)) return false;

    return true;
}

std::string ProfileManager::BuildPayload(const ProfileData& p)
{
    return p.id + "," +
        std::to_string(p.mmr) + "," +
        std::to_string(p.rank) + "," +
        std::to_string(p.rankScore) + "," +
        std::to_string(p.level) + "," +
        std::to_string(p.exp) + "," +
        std::to_string(p.win) + "," +
        std::to_string(p.lose);
}
