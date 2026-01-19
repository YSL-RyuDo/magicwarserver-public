#include "UserManager.h"
#include "DBManager.h"
#include "PasswordHasher.h"
#include <mysql/mysql.h>
#include <iostream>

std::mutex UserManager::s_mtx;
std::unordered_set<std::string> UserManager::s_loggedIn;
std::unordered_map<int, CardInfo> UserManager::s_cards;
std::unordered_map<int, RankInfo> UserManager::s_ranks;

// 계정
bool UserManager::AddUser(
    const std::string& id,
    const std::string& hash,
    const std::string& salt,
    const std::string& nick)
{
    std::lock_guard<std::mutex> lock(DBManager::GetMutex());
    MYSQL* db = DBManager::Get();

    if (IsIDExist(id) || IsNickExist(nick))
        return false;

    mysql_query(db, "START TRANSACTION");

    auto rollback = [&]() {
        mysql_query(db, "ROLLBACK");
        return false;
        };

    if (mysql_query(db,
        ("INSERT INTO user_account(id,hash,salt,nickname) VALUES('"
            + id + "','" + hash + "','" + salt + "','" + nick + "')").c_str()) != 0)
        return rollback();

    if (mysql_query(db,
        ("INSERT INTO user_info(user_id) VALUES('" + id + "')").c_str()) != 0)
        return rollback();

    if (mysql_query(db,
        ("INSERT INTO user_rank(user_id) VALUES('" + id + "')").c_str()) != 0)
        return rollback();

    if (mysql_query(db,
        ("INSERT INTO user_magic(user_id,magic0,magic1,magic2,magic3)"
            " VALUES('" + id + "',0,1,2,3)").c_str()) != 0)
        return rollback();

    if (mysql_query(db, "COMMIT") != 0)
        return rollback();

    return ValidateUser(id);
}

bool UserManager::CheckLogin(
    const std::string& id,
    const std::string& pw,
    std::string& outNick)
{
    std::lock_guard<std::mutex> lock(DBManager::GetMutex());
    MYSQL* db = DBManager::Get();

    if (mysql_query(db,
        ("SELECT hash,salt,nickname FROM user_account WHERE id='" + id + "'").c_str()) != 0)
        return false;

    MYSQL_RES* res = mysql_store_result(db);
    if (!res) return false;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    {
        mysql_free_result(res);
        return false;
    }

    std::string hash = row[0];
    std::string salt = row[1];
    outNick = row[2];

    mysql_free_result(res);
    return PasswordHasher::Hash(pw, salt) == hash;
}

bool UserManager::IsIDExist(const std::string& id)
{
    MYSQL* db = DBManager::Get();
    if (mysql_query(db,
        ("SELECT 1 FROM user_account WHERE id='" + id + "' LIMIT 1").c_str()) != 0)
        return false;

    MYSQL_RES* res = mysql_store_result(db);
    bool exist = (res && mysql_num_rows(res) > 0);
    if (res) mysql_free_result(res);
    return exist;
}

bool UserManager::IsNickExist(const std::string& nick)
{
    MYSQL* db = DBManager::Get();
    if (mysql_query(db,
        ("SELECT 1 FROM user_account WHERE nickname='" + nick + "' LIMIT 1").c_str()) != 0)
        return false;

    MYSQL_RES* res = mysql_store_result(db);
    bool exist = (res && mysql_num_rows(res) > 0);
    if (res) mysql_free_result(res);
    return exist;
}

// 로그인 상태
bool UserManager::TryLoginUser(const std::string& id)
{
    std::lock_guard<std::mutex> lock(s_mtx);
    if (s_loggedIn.count(id)) return false;
    s_loggedIn.insert(id);
    return true;
}

void UserManager::LogoutUser(const std::string& id)
{
    std::lock_guard<std::mutex> lock(s_mtx);
    s_loggedIn.erase(id);
}

// 저장 및 불러오기
bool UserManager::LoadUserInfo(const std::string& id, UserInfoData& out)
{
    MYSQL* db = DBManager::Get();
    if (mysql_query(db,
        ("SELECT level,exp,win,lose FROM user_info WHERE user_id='" + id + "'").c_str()) != 0)
        return false;

    MYSQL_RES* res = mysql_store_result(db);
    if (!res) return false;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    {
        mysql_free_result(res);
        return false;
    }

    out.level = atoi(row[0]);
    out.exp = atoi(row[1]);
    out.win = atoi(row[2]);
    out.lose = atoi(row[3]);

    mysql_free_result(res);
    return true;
}

bool UserManager::SaveUserInfo(const std::string& id, const UserInfoData& d)
{
    MYSQL* db = DBManager::Get();
    return mysql_query(db,
        ("UPDATE user_info SET "
            "level=" + std::to_string(d.level) +
            ",exp=" + std::to_string(d.exp) +
            ",win=" + std::to_string(d.win) +
            ",lose=" + std::to_string(d.lose) +
            " WHERE user_id='" + id + "'").c_str()) == 0;
}

bool UserManager::LoadUserRank(const std::string& id, UserRankData& out)
{
    MYSQL* db = DBManager::Get();
    if (mysql_query(db,
        ("SELECT mmr,rank,rank_score FROM user_rank WHERE user_id='" + id + "'").c_str()) != 0)
        return false;

    MYSQL_RES* res = mysql_store_result(db);
    if (!res) return false;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    {
        mysql_free_result(res);
        return false;
    }

    out.mmr = atoi(row[0]);
    out.rank = atoi(row[1]);
    out.rankScore = atoi(row[2]);

    mysql_free_result(res);
    return true;
}

bool UserManager::SaveUserRank(const std::string& id, const UserRankData& d)
{
    MYSQL* db = DBManager::Get();
    return mysql_query(db,
        ("UPDATE user_rank SET "
            "mmr=" + std::to_string(d.mmr) +
            ",rank=" + std::to_string(d.rank) +
            ",rank_score=" + std::to_string(d.rankScore) +
            " WHERE user_id='" + id + "'").c_str()) == 0;
}

bool UserManager::LoadUserMagic(const std::string& id, std::vector<int>& out)
{
    MYSQL* db = DBManager::Get();
    if (mysql_query(db,
        ("SELECT magic0,magic1,magic2,magic3 FROM user_magic WHERE user_id='" + id + "'").c_str()) != 0)
        return false;

    MYSQL_RES* res = mysql_store_result(db);
    if (!res) return false;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    {
        mysql_free_result(res);
        return false;
    }

    out = {
        atoi(row[0]),
        atoi(row[1]),
        atoi(row[2]),
        atoi(row[3])
    };

    mysql_free_result(res);
    return true;
}

//bool UserManager::ReplaceUserMagic(
//    const std::string& id, int oldCard, int newCard)
//{
//    MYSQL* db = DBManager::Get();
//    return mysql_query(db,
//        ("UPDATE user_magic SET "
//            "magic0=IF(magic0=" + std::to_string(oldCard) + "," + std::to_string(newCard) + ",magic0),"
//            "magic1=IF(magic1=" + std::to_string(oldCard) + "," + std::to_string(newCard) + ",magic1),"
//            "magic2=IF(magic2=" + std::to_string(oldCard) + "," + std::to_string(newCard) + ",magic2),"
//            "magic3=IF(magic3=" + std::to_string(oldCard) + "," + std::to_string(newCard) + ",magic3)"
//            " WHERE user_id='" + id + "'").c_str()) == 0;
//}

bool UserManager::ReplaceUserMagic(
    const std::string& id, int oldCard, int newCard)
{
    MYSQL* db = DBManager::Get();

    // 1. 현재 덱 로드
    std::string select =
        "SELECT magic0, magic1, magic2, magic3 "
        "FROM user_magic WHERE user_id='" + id + "'";

    if (mysql_query(db, select.c_str()) != 0)
        return false;

    MYSQL_RES* res = mysql_store_result(db);
    if (!res)
        return false;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row)
    {
        mysql_free_result(res);
        return false;
    }

    int deck[4];
    for (int i = 0; i < 4; ++i)
        deck[i] = atoi(row[i]);

    mysql_free_result(res);

    // 2. old / new 위치 찾기
    int oldIdx = -1;
    int newIdx = -1;

    for (int i = 0; i < 4; ++i)
    {
        if (deck[i] == oldCard)
            oldIdx = i;
        if (deck[i] == newCard)
            newIdx = i;
    }

    if (oldIdx == -1)
        return false;

    // 같은 카드 → 그대로 성공
    if (oldCard == newCard)
        return true;

    // 3. 스왑 or 치환
    if (newIdx != -1)
    {
        std::swap(deck[oldIdx], deck[newIdx]);
    }
    else
    {
        deck[oldIdx] = newCard;
    }

    // 4. DB 반영
    std::string update =
        "UPDATE user_magic SET "
        "magic0=" + std::to_string(deck[0]) + ","
        "magic1=" + std::to_string(deck[1]) + ","
        "magic2=" + std::to_string(deck[2]) + ","
        "magic3=" + std::to_string(deck[3]) +
        " WHERE user_id='" + id + "'";

    return mysql_query(db, update.c_str()) == 0;
}


bool UserManager::LoadUserProfile(const std::string& id, UserProfileData& out)
{
    return LoadUserInfo(id, out.info) && LoadUserRank(id, out.rank);
}

// 카드 및 랭크 테이블
bool UserManager::LoadCardTable()
{
    MYSQL* db = DBManager::Get();
    if (mysql_query(db,
        "SELECT id,name,attack,cooldown,cost,effect_type,speed,radius,duration,amount FROM card_table") != 0)
        return false;

    MYSQL_RES* res = mysql_store_result(db);
    if (!res) return false;

    s_cards.clear();
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        CardInfo c{};
        c.id = atoi(row[0]);
        c.name = row[1];
        c.attack = atoi(row[2]);
        c.cooldown = atoi(row[3]);
        c.cost = atoi(row[4]);
        c.effectType = row[5];
        c.speed = (float)atof(row[6]);
        c.radius = (float)atof(row[7]);
        c.duration = (float)atof(row[8]);
        c.amount = atoi(row[9]);
        s_cards[c.id] = c;
    }

    mysql_free_result(res);
    return true;
}

bool UserManager::LoadRankTable()
{
    MYSQL* db = DBManager::Get();
    if (mysql_query(db,
        "SELECT rank,name,min_score,max_score FROM rank_table") != 0)
        return false;

    MYSQL_RES* res = mysql_store_result(db);
    if (!res) return false;

    s_ranks.clear();
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        RankInfo r{};
        r.rank = atoi(row[0]);
        r.name = row[1];
        r.minScore = atoi(row[2]);
        r.maxScore = atoi(row[3]);
        s_ranks[r.rank] = r;
    }

    mysql_free_result(res);
    return true;
}

const CardInfo* UserManager::GetCardInfo(int id)
{
    auto it = s_cards.find(id);
    return it == s_cards.end() ? nullptr : &it->second;
}

const RankInfo* UserManager::GetRankInfo(int rank)
{
    auto it = s_ranks.find(rank);
    return it == s_ranks.end() ? nullptr : &it->second;
}

bool UserManager::ValidateUser(const std::string& id)
{
    UserInfoData info{};
    UserRankData rank{};
    std::vector<int> magic;

    return LoadUserInfo(id, info)
        && LoadUserRank(id, rank)
        && LoadUserMagic(id, magic);
}
