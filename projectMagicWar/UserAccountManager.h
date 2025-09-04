#pragma once
#include <string>
#include <vector>
#include <mutex>

struct UserAccount {
    std::string id;
    std::string password;
    std::string nickname;
};

class UserAccountManager {
private:
    std::string filePath;
    static std::mutex fileMutex;

    // 락 없는 내부 로드 (addAccount 등에서 사용)
    bool loadUnlocked(std::vector<UserAccount>& accounts);

public:
    UserAccountManager(const std::string& path);

    // 외부에서 안전하게 호출할 때
    bool load(std::vector<UserAccount>& accounts);

    // 계정 추가
    bool addAccount(const UserAccount& newAccount, int& errorType);

    // 로그인 확인
    bool checkLogin(const std::string& id, const std::string& password, std::string& nickname);
};
