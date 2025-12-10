#include "UserAccountManager.h"
#include "PacketType.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::mutex UserAccountManager::fileMutex_;

UserAccountManager::UserAccountManager(const std::string& filePath)
    : path_(filePath)
{
}

// 락 없이 읽기(내부 사용)
bool UserAccountManager::LoadUnlocked(std::vector<UserAccount>& out)
{
    std::ifstream in(path_);
    if (!in.is_open()) return false;

    out.clear();
    std::string line;

    bool first = true;
    while (std::getline(in, line))
    {
        if (first) { first = false; continue; }  // header skip
        if (line.empty()) continue;

        std::stringstream ss(line);

        std::string id, pw, nick;
        std::getline(ss, id, ',');
        std::getline(ss, pw, ',');
        std::getline(ss, nick, ',');

        out.push_back({ id, pw, nick });
    }
    return true;
}

// 락 걸고 읽기
bool UserAccountManager::Load(std::vector<UserAccount>& out)
{
    std::lock_guard<std::mutex> lock(fileMutex_);
    return LoadUnlocked(out);
}

// 회원가입
bool UserAccountManager::AddAccount(const UserAccount& acc, int& errorType)
{
    std::lock_guard<std::mutex> lock(fileMutex_);

    std::vector<UserAccount> accounts;
    LoadUnlocked(accounts);   // deadlock 없음

    for (auto& a : accounts)
    {
        if (a.id == acc.id)
        {
            errorType = PKT_SIGNUP_ID_EXISTS;
            return false;
        }
        if (a.nickname == acc.nickname)
        {
            errorType = PKT_SIGNUP_NICK_EXISTS;
            return false;
        }
    }

    bool exists = !accounts.empty();
    std::ofstream out;

    if (exists)
        out.open(path_, std::ios::app);
    else
    {
        out.open(path_);
        out << "ID,Password,Nickname\n";
    }

    if (!out.is_open())
    {
        std::cerr << "[UserAccountManager] 파일쓰기 실패\n";
        errorType = PKT_SIGNUP_ID_EXISTS;
        return false;
    }

    out << acc.id << "," << acc.password << "," << acc.nickname << "\n";
    out.close();

    errorType = PKT_SIGNUP_OK;
    return true;
}

// 로그인
bool UserAccountManager::CheckLogin(const std::string& id, const std::string& pw, std::string& nickname)
{
    std::lock_guard<std::mutex> lock(fileMutex_);

    std::vector<UserAccount> accounts;
    LoadUnlocked(accounts);

    for (auto& a : accounts)
    {
        if (a.id == id)
        {
            if (a.password == pw)
            {
                nickname = a.nickname;
                return true;
            }
            else
                return false; // pw mismatch
        }
    }
    return false; // no ID
}
