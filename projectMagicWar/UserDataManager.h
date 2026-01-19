#pragma once
#include <string>
#include <mutex>

class UserDataManager
{
public:
    UserDataManager(
        const std::string& statsPath,
        const std::string& magicPath,
        const std::string& recordPath);

    bool CreateUserData(const std::string& id);

private:
    static bool FileExists(const std::string& path);
    static void AppendLine(const std::string& path,
        const std::string& header,
        const std::string& line);

private:
    std::string statsPath_;
    std::string magicPath_;
    std::string recordPath_;

    static std::mutex fileMutex_;
};
