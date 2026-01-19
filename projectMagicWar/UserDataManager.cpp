#include "UserDataManager.h"
#include <fstream>
#include <iostream>

std::mutex UserDataManager::fileMutex_;

UserDataManager::UserDataManager(
    const std::string& statsPath,
    const std::string& magicPath,
    const std::string& recordPath)
    : statsPath_(statsPath)
    , magicPath_(magicPath)
    , recordPath_(recordPath)
{
}

bool UserDataManager::FileExists(const std::string& path)
{
    std::ifstream f(path);
    return f.good();
}

void UserDataManager::AppendLine(
    const std::string& path,
    const std::string& header,
    const std::string& line)
{
    std::lock_guard<std::mutex> lock(fileMutex_);

    bool exists = FileExists(path);
    std::ofstream out;

    if (exists)
    {
        out.open(path, std::ios::app);
    }
    else
    {
        out.open(path);
        out << header << "\n";
    }

    if (!out.is_open())
    {
        std::cerr << "[UserDataManager] 파일 열기 실패: " << path << "\n";
        return;
    }

    out << line << "\n";
    out.close();
}

bool UserDataManager::CreateUserData(const std::string& id)
{
    // MMR, RANK, RANKSCORE 초기화
    AppendLine(
        statsPath_,
        "ID,MMR,RANK,RANKSCORE",
        id + ",1000,0,0"
    );

    // MAGIC 4개 초기값 설정
    AppendLine(
        magicPath_,
        "ID,MAGIC0,MAGIC1,MAGIC2,MAGIC3",
        id + ",0,1,2,3"
    );

    // LEVEL, EXP, WIN, LOSE 초기화
    AppendLine(
        recordPath_,
        "ID,LEVEL,EXP,WIN,LOSE",
        id + ",1,0,0,0"
    );

    std::cout << "[UserDataManager] 유저 데이터 생성 완료 → " << id << "\n";
    return true;
}
