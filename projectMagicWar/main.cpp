// SimpleServer.cpp
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
typedef int socklen_t;
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
#endif

void handleClient(SOCKET clientSock) {
    char buffer[1024];
    while (true) {
        int recvSize = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (recvSize <= 0) {
            std::cout << "Client disconnected\n";
            break;
        }
        buffer[recvSize] = '\0';

        std::string msg(buffer);
        std::cout << "Received: " << msg << std::endl;

        std::string reply = "Server reply: " + msg;
        send(clientSock, reply.c_str(), (int)reply.size(), 0);
    }
#ifdef _WIN32
    closesocket(clientSock);
#else
    close(clientSock);
#endif
}

int main() {
#ifdef _WIN32
    // Set console to UTF-8
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(7777);  // Server port
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed\n";
        return 1;
    }

    listen(serverSock, 5);
    std::cout << "Server running... port 7777\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientSize = sizeof(clientAddr);
        SOCKET clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientSize);
        if (clientSock != INVALID_SOCKET) {
            std::cout << "Client connected!\n";
            std::thread(handleClient, clientSock).detach();
        }
    }

#ifdef _WIN32
    closesocket(serverSock);
    WSACleanup();
#else
    close(serverSock);
#endif
    return 0;
}
