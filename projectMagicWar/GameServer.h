#pragma once
#include <vector>
#include <thread>
#include <mutex>
#include "Client.h"
#include "MatchManager.h"

class GameServer {
private:
    SOCKET serverSock;
    std::vector<Client> clients;
    std::unordered_map<std::string, Client> clientsMap;
    std::mutex clientsMutex;
    int port;
    MatchManager matchManager;
public:
    GameServer(int port);
    ~GameServer();

    void start();
    void acceptClients();
    void handleClient(Client client);
};
