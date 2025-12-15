#include "DBManager.h"
#include <iostream>

MYSQL* DBManager::s_conn = nullptr;
std::mutex DBManager::s_mtx;

bool DBManager::Init(
    const std::string& host,
    const std::string& user,
    const std::string& password,
    const std::string& db,
    unsigned int port)
{
    s_conn = mysql_init(nullptr);
    if (!s_conn)
        return false;

    mysql_options(s_conn, MYSQL_DEFAULT_AUTH, "mysql_native_password");

    if (!mysql_real_connect(
        s_conn,
        host.c_str(),
        user.c_str(),
        password.c_str(),
        db.c_str(),
        port,
        nullptr,
        0))
    {
        std::cout << "[DB ERROR] " << mysql_error(s_conn) << "\n";
        return false;
    }

    mysql_set_character_set(s_conn, "utf8mb4");
    std::cout << "[DB] Connected\n";
    return true;
}

MYSQL* DBManager::Get()
{
    return s_conn;
}

std::mutex& DBManager::GetMutex()
{
    return s_mtx;
}
