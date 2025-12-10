#pragma once
#include <string>
#include <vector>
#include <mutex>

struct UserAccount
{
    std::string id;
    std::string password;
    std::string nickname;
};

class UserAccountManager
{
public:
    UserAccountManager(const std::string& filePath);

    bool AddAccount(const UserAccount& acc, int& errorType);
    bool CheckLogin(const std::string& id, const std::string& pw, std::string& nickname);

private:
    bool LoadUnlocked(std::vector<UserAccount>& out);
    bool Load(std::vector<UserAccount>& out);

private:
    std::string path_;
    static std::mutex fileMutex_;
};
