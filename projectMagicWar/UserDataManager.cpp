#include "UserDataManager.h"
#include <fstream>
#include <iostream>

std::mutex UserDataManager::fileMutex;

bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

void UserDataManager::appendLine(const std::string& path, const std::string& header, const std::string& line) {
    std::lock_guard<std::mutex> lock(fileMutex);

    bool fileExistsFlag = fileExists(path);
    std::ofstream out;

    if (fileExistsFlag) {
        out.open(path, std::ios::app);
    }
    else {
        out.open(path);
        out << header << "\n";
    }

    if (!out.is_open()) {
        std::cerr << "[파일 열기 실패] " << path << std::endl;
        return;
    }

    out << line << "\n";
    out.close();
}

void UserDataManager::createUserData(const std::string& id) {
    appendLine("UserStats.csv", "ID,MMR,RANK,RANKSCORE", id + ",1000,0,0");
    appendLine("UserMagic.csv", "ID,MAGIC0,MAGIC1,MAGIC2,MAGIC3", id + ",0,1,2,3");
    appendLine("UserRecord.csv", "ID,LEVEL,EXP,WIN,LOSE", id + ",1,0,0,0");

    std::cout << "[유저 데이터 생성 완료] ID=" << id << std::endl;
}
