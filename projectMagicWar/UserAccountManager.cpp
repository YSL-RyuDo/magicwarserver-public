#include "UserAccountManager.h"
#include "PacketType.h"
#include <fstream>
#include <sstream>
#include <iostream>

std::mutex UserAccountManager::fileMutex;

UserAccountManager::UserAccountManager(const std::string& path)
    : filePath(path) {
}

bool UserAccountManager::loadUnlocked(std::vector<UserAccount>& accounts) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    accounts.clear();
    std::string line;
    bool firstLine = true;
    while (std::getline(in, line)) {
        if (firstLine) { firstLine = false; continue; }
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string id, pw, nick;
        std::getline(ss, id, ',');
        std::getline(ss, pw, ',');
        std::getline(ss, nick, ',');

        accounts.push_back({ id, pw, nick });
    }
    return true;
}

bool UserAccountManager::load(std::vector<UserAccount>& accounts) {
    std::lock_guard<std::mutex> lock(fileMutex);
    return loadUnlocked(accounts);
}

bool UserAccountManager::addAccount(const UserAccount& newAccount, int& errorType) {
    std::lock_guard<std::mutex> lock(fileMutex);

    std::vector<UserAccount> accounts;
    loadUnlocked(accounts); // deadlock 없음

    for (auto& acc : accounts) {
        if (acc.id == newAccount.id) {
            errorType = PacketType::SignupIDError;
            return false;
        }
        if (acc.nickname == newAccount.nickname) {
            errorType = PacketType::SignupNickError;
            return false;
        }
    }

    bool fileExists = !accounts.empty();
    std::ofstream out;

    if (fileExists) out.open(filePath, std::ios::app);
    else {
        out.open(filePath);
        out << "ID,Password,NickName\n";
    }

    if (!out.is_open()) {
        errorType = PacketType::SignupIDError;
        return false;
    }

    out << newAccount.id << "," << newAccount.password << "," << newAccount.nickname << "\n";
    out.close();

    errorType = PacketType::SignupSuccess;
    return true;
}

bool UserAccountManager::checkLogin(const std::string& id, const std::string& password, std::string& nickname) {
    std::lock_guard<std::mutex> lock(fileMutex);

    std::vector<UserAccount> accounts;
    loadUnlocked(accounts);

    for (auto& acc : accounts) {
        if (acc.id == id) {
            if (acc.password == password) {
                nickname = acc.nickname;
                return true;
            }
            else {
                return false; // PW 불일치
            }
        }
    }
    return false; // ID 없음
}
