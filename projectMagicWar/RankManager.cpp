#include "RankManager.h"
#include "Utils.h"
#include <fstream>

std::vector<RankInfo> RankManager::ranks_;

bool RankManager::Load(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::string line;
    std::getline(file, line); // header skip

    while (std::getline(file, line))
    {
        auto cols = SplitCSV(line);
        if (cols.size() < 4) continue;

        RankInfo info;
        info.rank = std::stoul(cols[0]);
        info.name = cols[1];
        info.minScore = std::stoul(cols[2]);
        info.maxScore = std::stoul(cols[3]);

        ranks_.push_back(info);
    }
    return true;
}

bool RankManager::GetRankInfo(uint32_t rank, RankInfo& out)
{
    for (auto& r : ranks_)
    {
        if (r.rank == rank)
        {
            out = r;
            return true;
        }
    }
    return false;
}
