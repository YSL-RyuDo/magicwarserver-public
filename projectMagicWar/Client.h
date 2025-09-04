#pragma once
#include <string>
#include <winsock2.h>

struct Client {
    SOCKET socket;
    std::string ip;
    int port;
    std::string id;
    std::string nickname;
};
