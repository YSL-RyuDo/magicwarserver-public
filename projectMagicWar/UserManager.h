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
    static bool AddUser(const string& id, const string& hash, const string& salt, const string& nick);

    // 아이디 존재 확인
    static bool IsIDExist(const string& id);

    // 닉네임 존재 여부
    static bool IsNickExist(const string& nick);

    // 로그인용 전체 데이터 로드
    static bool LoadUser(const string& id, string& outHash, string& outSalt, string& outNick);

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