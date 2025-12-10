#pragma once
#include <string>
#include <mutex>
#include <vector>
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
    static bool AddUser(const string& id, const string& pw, const string& nick);

    // 아이디 존재 확인
    static bool IsIDExist(const string& id);

    // 로그인
    static bool CheckLogin(const string& id, const string& pw, string& outNick);

    // 프로필 불러오기
    static bool LoadUserInfo(const string& nick, UserInfo& outInfo);

    // 승/패/경기 기록 업데이트
    static bool UpdateRecord(const string& nick, const string& resultType);

private:
    static mutex s_mtx;
    static string s_userFile;
    static string s_infoFile;
};
