#pragma once

#include <string>
#include "Session.h"

class PacketParser
{
public:
    // RecvCompletion 후 Session.recvBuffer에 쌓인 모든 패킷을 분해하여 PacketManager로 넘긴다.
    static void ProcessBuffer(Session* session);

private:
    //static bool ExtractPacket(Session* session, int& outPacketId, std::string& outBody);
};
