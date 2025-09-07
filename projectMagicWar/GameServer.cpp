#include "GameServer.h"
#include "PacketManager.h"
#include <iostream>
#include <unordered_map>
#include <algorithm>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")



GameServer::GameServer(int port) : port(port) {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        throw std::runtime_error("소켓 생성 실패");
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        throw std::runtime_error("바인드 실패");
    }

    listen(serverSock, 5);
    std::cout << "게임 서버 실행 중... 포트 " << port << std::endl;

    matchManager.Init(&this->clientsMap);
    PacketManager::Init(&this->clientsMap, &this->matchManager);
}

GameServer::~GameServer() {
    closesocket(serverSock);
    WSACleanup();
}

void GameServer::start() {
    std::thread([this]() {
        while (true) {
            matchManager.Update();   // 매칭 자동 검사
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        }).detach();

    acceptClients(); // 기존 클라 accept 루프
}

void GameServer::acceptClients() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientSize = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientSize);

        if (clientSock != INVALID_SOCKET) {
            char ipStr[46];
            InetNtopA(AF_INET, &(clientAddr.sin_addr), ipStr, 46);
            int clientPort = ntohs(clientAddr.sin_port);

            Client newClient{ clientSock, ipStr, clientPort };
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                clients.push_back(newClient);
            }

            std::cout << "클라이언트 접속: " << ipStr << ":" << clientPort << std::endl;
            std::thread(&GameServer::handleClient, this, newClient).detach();
        }
    }
}

void GameServer::handleClient(Client client) {
    char buffer[1024];
    while (true) {
        int recvSize = recv(client.socket, buffer, sizeof(buffer), 0);
        if (recvSize <= 0) {
            std::cout << "클라이언트 종료: " << client.ip << ":" << client.port << std::endl;
            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                clients.erase(std::remove_if(clients.begin(), clients.end(),
                    [&](Client& c) { return c.socket == client.socket; }), clients.end());
            }
            closesocket(client.socket);
            break;
        }

        if (recvSize < sizeof(int)) continue;

        int packetId = 0;
        memcpy(&packetId, buffer, sizeof(int));
        std::string rawMsg(buffer + sizeof(int), recvSize - sizeof(int));

        PacketManager::handlePacket(client, packetId, rawMsg);
    }
}
