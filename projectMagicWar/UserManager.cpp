#include "UserManager.h"
#include "DBManager.h"
#include "PasswordHasher.h"
#include "PreparedQuery.h"
#include <iostream>

std::mutex UserManager::s_mtx;
std::unordered_set<std::string> UserManager::s_loggedIn;
std::unordered_map<int, CardInfo> UserManager::s_cards;
std::unordered_map<int, RankInfo> UserManager::s_ranks;

// Runtime queries are owned by the single DBWorker. Startup table loads finish
// before network workers start. Do not call these methods from IOCP workers.
namespace {
using Rows = std::vector<std::vector<std::string>>;
bool Query(const char* sql, const std::vector<std::string>& values, Rows* rows = nullptr)
{
    return PreparedQuery(DBManager::Get(), sql, values, rows);
}
}

bool UserManager::AddUser(const std::string& id, const std::string& hash,
    const std::string& salt, const std::string& nick)
{
    MYSQL* db = DBManager::Get();
    if (!db || mysql_query(db, "START TRANSACTION") != 0) return false;
    // The schema must enforce UNIQUE id/nickname; any insert error rolls back.
    const bool ok =
        Query("INSERT INTO user_account(id,hash,salt,nickname) VALUES(?,?,?,?)",
            {id, hash, salt, nick}) &&
        Query("INSERT INTO user_info(user_id) VALUES(?)", {id}) &&
        Query("INSERT INTO user_rank(user_id) VALUES(?)", {id}) &&
        Query("INSERT INTO user_magic(user_id,magic0,magic1,magic2,magic3) VALUES(?,0,1,2,3)", {id}) &&
        ValidateUser(id);
    if (!ok || mysql_query(db, "COMMIT") != 0)
    {
        mysql_query(db, "ROLLBACK");
        return false;
    }
    return true;
}

bool UserManager::CheckLogin(const std::string& id, const std::string& pw,
    std::string& outNick)
{
    Rows rows;
    if (!Query("SELECT hash,salt,nickname FROM user_account WHERE id=?", {id}, &rows)
        || rows.empty()) return false;
    if (PasswordHasher::Hash(pw, rows[0][1]) != rows[0][0]) return false;
    outNick = rows[0][2];
    return true;
}

bool UserManager::IsIDExist(const std::string& id)
{
    Rows rows;
    return Query("SELECT 1 FROM user_account WHERE id=? LIMIT 1", {id}, &rows)
        && !rows.empty();
}
bool UserManager::IsNickExist(const std::string& nick)
{
    Rows rows;
    return Query("SELECT 1 FROM user_account WHERE nickname=? LIMIT 1", {nick}, &rows)
        && !rows.empty();
}

bool UserManager::TryLoginUser(const std::string& id)
{
    std::lock_guard<std::mutex> lock(s_mtx);
    return s_loggedIn.insert(id).second;
}
void UserManager::LogoutUser(const std::string& id)
{
    std::lock_guard<std::mutex> lock(s_mtx);
    s_loggedIn.erase(id);
}

bool UserManager::LoadUserInfo(const std::string& id, UserInfoData& out)
{
    Rows r;
    if (!Query("SELECT level,exp,win,lose FROM user_info WHERE user_id=?", {id}, &r)
        || r.empty()) return false;
    out = {std::stoi(r[0][0]), std::stoi(r[0][1]), std::stoi(r[0][2]), std::stoi(r[0][3])};
    return true;
}
bool UserManager::SaveUserInfo(const std::string& id, const UserInfoData& d)
{
    return Query("UPDATE user_info SET level=?,exp=?,win=?,lose=? WHERE user_id=?",
        {std::to_string(d.level), std::to_string(d.exp), std::to_string(d.win),
            std::to_string(d.lose), id});
}
bool UserManager::LoadUserRank(const std::string& id, UserRankData& out)
{
    Rows r;
    if (!Query("SELECT mmr,`rank`,rank_score FROM user_rank WHERE user_id=?", {id}, &r)
        || r.empty()) return false;
    out = {std::stoi(r[0][0]), std::stoi(r[0][1]), std::stoi(r[0][2])};
    return true;
}
bool UserManager::SaveUserRank(const std::string& id, const UserRankData& d)
{
    return Query("UPDATE user_rank SET mmr=?,`rank`=?,rank_score=? WHERE user_id=?",
        {std::to_string(d.mmr), std::to_string(d.rank), std::to_string(d.rankScore), id});
}
bool UserManager::LoadUserMagic(const std::string& id, std::vector<int>& out)
{
    Rows r;
    if (!Query("SELECT magic0,magic1,magic2,magic3 FROM user_magic WHERE user_id=?", {id}, &r)
        || r.empty()) return false;
    out.clear();
    for (const auto& v : r[0]) out.push_back(std::stoi(v));
    return true;
}
bool UserManager::ReplaceUserMagic(const std::string& id, int oldCard, int newCard)
{
    if (!GetCardInfo(newCard)) return false;
    std::vector<int> deck;
    if (!LoadUserMagic(id, deck)) return false;
    int oldIdx = -1, newIdx = -1;
    for (int i = 0; i < 4; ++i)
    {
        if (deck[i] == oldCard) oldIdx = i;
        if (deck[i] == newCard) newIdx = i;
    }
    if (oldIdx < 0) return false;
    if (oldCard == newCard) return true;
    if (newIdx >= 0) std::swap(deck[oldIdx], deck[newIdx]);
    else deck[oldIdx] = newCard;
    return Query("UPDATE user_magic SET magic0=?,magic1=?,magic2=?,magic3=? WHERE user_id=?",
        {std::to_string(deck[0]), std::to_string(deck[1]), std::to_string(deck[2]),
            std::to_string(deck[3]), id});
}
bool UserManager::LoadUserProfile(const std::string& id, UserProfileData& out)
{
    return LoadUserInfo(id, out.info) && LoadUserRank(id, out.rank);
}

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
        "SELECT `rank`,name,min_score,max_score FROM rank_table") != 0)
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
