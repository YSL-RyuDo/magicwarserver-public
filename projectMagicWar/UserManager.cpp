#include <fstream>
#include <sstream>
#include <iostream>

#include "UserManager.h"
#include "PasswordHasher.h"
#include "Utils.h"

using namespace std;

static const char* USER_CSV = "UserAccount.csv";
static const char* INFO_PATH = "UserInfo.csv";
static const char* MAGIC_PATH = "UserMagic.csv";
static const char* RANK_PATH = "UserRank.csv";

mutex UserManager::s_mtx;

string UserManager::s_userFile = USER_CSV;
string UserManager::s_infoFile = "";
// UserManager.cpp (전역 영역)
std::unordered_set<std::string> UserManager::s_loggedInUsers;


void UserManager::Init(const string& userFile, const string& infoFile)
{
    lock_guard<mutex> lock(s_mtx);
    s_userFile = userFile;
    s_infoFile = infoFile;
}

/*
    CSV 구조 변경됨:
    ID,Hash,Salt,Nick
*/

bool UserManager::AddUser(const string& id, const string& hash, const string& salt, const string& nick)
{
    lock_guard<mutex> lock(s_mtx);

    ifstream fin(s_userFile);
    if (fin.is_open())
    {
        string line;
        getline(fin, line);

        while (getline(fin, line))
        {
            stringstream ss(line);
            string cid, chash, csalt, cnick;

            getline(ss, cid, ',');
            getline(ss, chash, ',');
            getline(ss, csalt, ',');
            getline(ss, cnick, ',');

            if (cid == id)
                return false; // ID 중복
        }
        fin.close();
    }

    ofstream fout(s_userFile, ios::app);
    if (!fout.is_open())
    {
        cout << "[UserManager] CSV 쓰기 실패: " << s_userFile << endl;
        return false;
    }

    fout << id << "," << hash << "," << salt << "," << nick << "\n";
    return true;
}

void UserManager::CreateUserInfo(const std::string& id)
{
    std::ofstream file(INFO_PATH, std::ios::app);
    if (!file.is_open())
    {
        std::cout << "[UserInfo] 파일 열기 실패\n";
        return;
    }
    file << id << ",1,0,0,0\n";
}

void UserManager::CreateUserMagic(const std::string& id)
{
    std::ofstream file(MAGIC_PATH, std::ios::app);
    if (!file.is_open())
    {
        std::cout << "[UserMagic] 파일 열기 실패\n";
        return;
    }
    // 기본 스킬 0,1,2,3
    file << id << ",0,1,2,3\n";
}

void UserManager::CreateUserRank(const std::string& id)
{
    std::ofstream file(RANK_PATH, std::ios::app);
    if (!file.is_open())
    {
        std::cout << "[UserRank] 파일 열기 실패\n";
        return;
    }
    // 기본 MMR=1000, RANK=0(브론즈), RANKSCORE=0
    file << id << ",1000,0,0\n";
}

bool UserManager::IsIDExist(const string& id)
{
    lock_guard<mutex> lock(s_mtx);

    ifstream fin(s_userFile);
    if (!fin.is_open()) return false;

    string line;
    getline(fin, line);

    while (getline(fin, line))
    {
        stringstream ss(line);
        string cid, chash, csalt, cnick;

        getline(ss, cid, ',');
        getline(ss, chash, ',');
        getline(ss, csalt, ',');
        getline(ss, cnick, ',');

        if (cid == id)
            return true;
    }
    return false;
}

bool UserManager::IsNickExist(const string& nick)
{
    lock_guard<mutex> lock(s_mtx);

    ifstream fin(s_userFile);
    if (!fin.is_open()) return false;

    string line;
    getline(fin, line); // 헤더 스킵

    while (getline(fin, line))
    {
        stringstream ss(line);
        string cid, chash, csalt, cnick;

        getline(ss, cid, ',');
        getline(ss, chash, ',');
        getline(ss, csalt, ',');
        getline(ss, cnick, ',');

        if (cnick == nick)
            return true;
    }
    return false;
}

bool UserManager::LoadUser(const string& id, string& outHash, string& outSalt, string& outNick)
{
    lock_guard<mutex> lock(s_mtx);

    ifstream fin(s_userFile);
    if (!fin.is_open()) return false;

    string line;
    getline(fin, line); // header skip

    while (getline(fin, line))
    {
        stringstream ss(line);
        string cid, chash, csalt, cnick;

        getline(ss, cid, ',');
        getline(ss, chash, ',');
        getline(ss, csalt, ',');
        getline(ss, cnick, ',');

        if (cid != id) continue;

        outHash = chash;
        outSalt = csalt;
        outNick = cnick;
        return true;
    }

    return false;
}


/*
    로그인 검증:
    1) CSV에서 ID 검색
    2) 저장된 Hash, Salt 가져옴
    3) 입력된 password + salt → 해시 계산
    4) 저장된 hash와 비교
*/
bool UserManager::CheckLogin(const string& id, const string& pw, string& outNick)
{
    lock_guard<mutex> lock(s_mtx);

    ifstream fin(s_userFile);
    if (!fin.is_open()) return false;

    string line;
    getline(fin, line);

    while (getline(fin, line))
    {
        stringstream ss(line);
        string cid, chash, csalt, cnick;

        getline(ss, cid, ',');
        getline(ss, chash, ',');
        getline(ss, csalt, ',');
        getline(ss, cnick, ',');

        if (cid != id) continue;

        // 1) 입력 PW 해시화
        string computed = PasswordHasher::Hash(pw, csalt);

        // 2) 비교
        if (computed != chash)
            return false;

        outNick = cnick;
        return true;
    }

    return false;
}

bool UserManager::LoadUserAccount(const std::string& id, std::string& outNick)
{
    std::ifstream file("UserAccount.csv");
    std::string line;
    std::getline(file, line);

    while (std::getline(file, line))
    {
        auto c = SplitCSV(line);
        if (c.size() < 4) continue;
        if (c[0] == id)
        {
            outNick = c[3];
            return true;
        }
    }
    return false;
}

bool UserManager::LoadUserInfo(const std::string& id, uint32_t& outLevel, uint32_t& outExp)
{
    std::ifstream file("UserInfo.csv");
    std::string line;
    std::getline(file, line);

    while (std::getline(file, line))
    {
        auto c = SplitCSV(line);
        if (c.size() < 3) continue;
        if (c[0] == id)
        {
            outLevel = std::stoul(c[1]);
            outExp = std::stoul(c[2]);
            return true;
        }
    }
    return false;
}

bool UserManager::LoadUserRank(const std::string& id, uint32_t& outRank, uint32_t& outRankScore)
{
    std::ifstream file("UserRank.csv");
    std::string line;
    std::getline(file, line);

    while (std::getline(file, line))
    {
        auto c = SplitCSV(line);
        if (c.size() < 4) continue;
        if (c[0] == id)
        {
            outRank = std::stoul(c[2]);
            outRankScore = std::stoul(c[3]);
            return true;
        }
    }
    return false;
}

bool UserManager::TryLoginUser(const std::string& id)
{
    lock_guard<mutex> lock(s_mtx);

    if (s_loggedInUsers.count(id) > 0)
        return false; // 이미 로그인 중

    s_loggedInUsers.insert(id);
    return true;
}

void UserManager::LogoutUser(const std::string& id)
{
    lock_guard<mutex> lock(s_mtx);
    s_loggedInUsers.erase(id);
}

