#pragma once
#include <mysql/mysql.h>
#include <string>
#include <mutex>
using namespace std;
class DBManager
{
public:
    static bool Init(const string& host,const string& user,const string& password,const string& db,unsigned int port = 3307);

    static MYSQL* Get();
    static mutex& GetMutex();

private:
    static MYSQL* s_conn;
    static mutex s_mtx;
};
