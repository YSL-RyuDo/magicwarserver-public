#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include "Client.h"

class GameServer {
private:
    SOCKET serverSock;
    std::vector<Client> clients;
    std::mutex clientsMutex;
    int port;

public:
    GameServer(int port);
    ~GameServer();

    void start();
    void acceptClients();
    void handleClient(Client client);
};
