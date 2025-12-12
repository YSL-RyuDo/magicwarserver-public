#pragma once
#include <string>
#include <mutex>
#include <vector>
#include<unordered_set>
using namespace std;

struct UserInfo
{
    string nickname;
    int win;
    int draw;
    int lose;
    int exp;
    int level;
};

class UserManager
{
public:
    static void Init(const string& userFile, const string& infoFile);

    // 회원가입
    static bool AddUser(const string& id, const string& hash, const string& salt, const string& nick);
    static void CreateUserInfo(const std::string& id);
    static void CreateUserMagic(const std::string& id);
    static void CreateUserRank(const std::string& id);

    // 아이디 존재 확인
    static bool IsIDExist(const string& id);

    // 닉네임 존재 여부
    static bool IsNickExist(const string& nick);

    // 로그인용 전체 데이터 로드
    static bool LoadUser(const string& id, string& outHash, string& outSalt, string& outNick);

    // 로그인
    static bool CheckLogin(const string& id, const string& pw, string& outNick);

    static bool LoadUserAccount(const std::string& id, std::string& outNick);

    static bool LoadUserInfo(const std::string& id, uint32_t& outLevel, uint32_t& outExp);

    static bool LoadUserRank(const std::string& id, uint32_t& outRank, uint32_t& outRankScore);

    static bool TryLoginUser(const std::string& id);
    static void LogoutUser(const std::string& id);

private:
    static mutex s_mtx;
    static string s_userFile;
    static string s_infoFile;
    static std::unordered_set<std::string> s_loggedInUsers;
    

};