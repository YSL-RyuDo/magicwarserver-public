#include "DBManager.h"
#include <iostream>

// DBManager에서 공유하는 MySQL 전역 커넥션 객체입니다.
MYSQL* DBManager::s_conn = nullptr;

// DB 접근 동기화를 위한 전역 뮤텍스입니다.
mutex DBManager::s_mtx;

// MySQL 서버에 연결하고 문자셋을 초기화하는 함수입니다.
bool DBManager::Init(
    const string& host,
    const string& user,
    const string& password,
    const string& db,
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

// 현재 사용 중인 MySQL 커넥션을 반환하는 함수입니다.
MYSQL* DBManager::Get()
{
    return s_conn;
}

// DB 접근 시 사용할 전역 뮤텍스를 반환하는 함수입니다.
std::mutex& DBManager::GetMutex()
{
    return s_mtx;
}
