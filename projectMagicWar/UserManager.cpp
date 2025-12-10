#include "UserManager.h"
#include "PasswordHasher.h"   // ★ 추가

#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;

static const char* USER_CSV = "UserAccount.csv";

mutex UserManager::s_mtx;

string UserManager::s_userFile = USER_CSV;
string UserManager::s_infoFile = "";

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
