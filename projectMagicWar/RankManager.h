#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct RankInfo
{
    uint32_t rank;
    std::string name;
    uint32_t minScore;
    uint32_t maxScore;
};

class RankManager
{
public:
    static bool Load(const std::string& path);
    static bool GetRankInfo(uint32_t rank, RankInfo& out);

private:
    static std::vector<RankInfo> ranks_;
};
