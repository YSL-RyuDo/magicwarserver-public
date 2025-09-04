#pragma once
#include <string>
#include <iostream>
#include "PacketType.h"
#include "Client.h"

class PacketManager {
public:
    static void sendPacket(SOCKET sock, int packetId, const std::string& msg);
    static void handlePacket(Client& client, int packetId, const std::string& rawMsg);
};
