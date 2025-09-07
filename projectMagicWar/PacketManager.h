#pragma once
#include <string>
#include <unordered_map>
#include "Client.h"
#include "PacketType.h"

class MatchManager; // 전방 선언

class PacketManager {
public:
    static void Init(std::unordered_map<std::string, Client>* clients, MatchManager* mm);
    static void handlePacket(const Client& client, int packetId, const std::string& rawMsg);
    static void sendPacket(SOCKET sock, int packetId, const std::string& msg);

private:
    static std::unordered_map<std::string, Client>* s_clients; // 선언만
    static MatchManager* s_match; // 선언만
};
