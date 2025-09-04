#include "PacketManager.h"
#include "UserAccountManager.h"
#include "UserDataManager.h"
void PacketManager::sendPacket(SOCKET sock, int packetId, const std::string& msg) {
    char header[sizeof(int)];
    memcpy(header, &packetId, sizeof(int));

    std::string packet(header, sizeof(int));
    packet += msg;

    send(sock, packet.c_str(), (int)packet.size(), 0);

    std::cout << "[서버→클라] ID=" << packetId
        << " 데이터=" << msg << std::endl;
}

void PacketManager::handlePacket(Client& client, int packetId, const std::string& rawMsg) {
    std::cout << "수신: ID=" << packetId << " 데이터=" << rawMsg << std::endl;

    switch (packetId) {
    case PacketType::Signup: {
        std::cout << "[회원가입 요청] " << rawMsg << std::endl;

        // rawMsg 형식: "id,password,nickname"
        size_t first = rawMsg.find(',');
        size_t second = rawMsg.find(',', first + 1);
        if (first == std::string::npos || second == std::string::npos) {
            PacketManager::sendPacket(client.socket, PacketType::SignupIDError, "잘못된 요청");
            break;
        }

        std::string id = rawMsg.substr(0, first);
        std::string pw = rawMsg.substr(first + 1, second - first - 1);
        std::string nickname = rawMsg.substr(second + 1);

        UserAccountManager mgr("UserAccount.csv");
        UserAccount newAcc{ id, pw, nickname };

        int resultType = PacketType::SignupSuccess;
        if (mgr.addAccount(newAcc, resultType)) {
            UserDataManager::createUserData(id);
            PacketManager::sendPacket(client.socket, resultType, id);
        }
        else {
            if (resultType == PacketType::SignupIDError) {
                PacketManager::sendPacket(client.socket, PacketType::SignupIDError, id);
            }
            else if (resultType == PacketType::SignupNickError) {
                PacketManager::sendPacket(client.socket, PacketType::SignupNickError, nickname);
            }
            else {
                // 여기선 그냥 "서버 내부 오류" 명확히 표시
                PacketManager::sendPacket(client.socket, -1, "Internal Error");
            }
        }
        break;
    }
    case PacketType::Login: {
        std::cout << "[로그인 요청] " << rawMsg << std::endl;

        size_t pos = rawMsg.find(',');
        if (pos == std::string::npos) {
            sendPacket(client.socket, PacketType::LoginIDError, "잘못된 요청");
            break;
        }

        std::string id = rawMsg.substr(0, pos);
        std::string pw = rawMsg.substr(pos + 1);

        UserAccountManager mgr("UserAccount.csv");
        std::vector<UserAccount> accounts;
        mgr.load(accounts);

        bool found = false;
        for (auto& acc : accounts) {
            if (acc.id == id) {
                found = true;
                if (acc.password == pw) {
                    sendPacket(client.socket, PacketType::LoginSuccess, acc.nickname);
                    std::cout << "[로그인 성공] ID=" << id << " NickName=" << acc.nickname << std::endl;
                }
                else {
                    sendPacket(client.socket, PacketType::LoginPWError, id);
                    std::cout << "[로그인 실패] PW 불일치 ID=" << id << std::endl;
                }
                break;
            }
        }

        if (!found) {
            sendPacket(client.socket, PacketType::LoginIDError, id);
            std::cout << "[로그인 실패] 존재하지 않는 ID=" << id << std::endl;
        }

        break;
    }


    default:
        std::cout << "[알 수 없는 패킷] " << packetId << std::endl;
        sendPacket(client.socket, -1, "알 수 없는 패킷");
        break;
    }
}
