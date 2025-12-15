#pragma once
#include <mysql/mysql.h>
#include <string>
#include <mutex>

class DBManager
{
public:
    static bool Init(
        const std::string& host,
        const std::string& user,
        const std::string& password,
        const std::string& db,
        unsigned int port = 3307
    );

    static MYSQL* Get();
    static std::mutex& GetMutex();

private:
    static MYSQL* s_conn;
    static std::mutex s_mtx;
};
