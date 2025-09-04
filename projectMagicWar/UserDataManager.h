#pragma once
#include <string>
#include <mutex>

class UserDataManager {
private:
    static std::mutex fileMutex;

    static void appendLine(const std::string& path, const std::string& header, const std::string& line);

public:
    static void createUserData(const std::string& id);
};
