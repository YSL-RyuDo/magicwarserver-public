#include "PacketManager.h"
#include "UserAccountManager.h"
#include "UserDataManager.h"
#include <fstream>   // ifstream, ofstream
#include <sstream>   // stringstream
#include <iostream>
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
    case PacketType::RequestProfile: {
        std::cout << "[프로필 요청] " << rawMsg << std::endl;
        std::string id = rawMsg;

        std::string nickname;
        std::string level;
        std::string rank, rankScore;

        // 1) UserAccount.csv → 닉네임
        {
            std::ifstream in("UserAccount.csv");
            if (in.is_open()) {
                std::string line;
                bool firstLine = true;
                while (std::getline(in, line)) {
                    if (firstLine) { firstLine = false; continue; }
                    if (line.empty()) continue;

                    std::stringstream ss(line);
                    std::string csvId, pw, nick;
                    std::getline(ss, csvId, ',');
                    std::getline(ss, pw, ',');
                    std::getline(ss, nick, ',');

                    if (csvId == id) {
                        nickname = nick;
                        break;
                    }
                }
                in.close();
            }
        }

        // 2) UserRecord.csv → LEVEL (exp 제거)
        {
            std::ifstream in("UserRecord.csv");
            if (in.is_open()) {
                std::string line;
                bool firstLine = true;
                while (std::getline(in, line)) {
                    if (firstLine) { firstLine = false; continue; }
                    if (line.empty()) continue;

                    std::stringstream ss(line);
                    std::string csvId, lvl, ex, win, lose;
                    std::getline(ss, csvId, ',');
                    std::getline(ss, lvl, ',');
                    std::getline(ss, ex, ',');
                    std::getline(ss, win, ',');
                    std::getline(ss, lose, ',');

                    if (csvId == id) {
                        level = lvl;
                        break;
                    }
                }
                in.close();
            }
        }

        // 3) UserStats.csv → RANK, RANKSCORE
        {
            std::ifstream in("UserStats.csv");
            if (in.is_open()) {
                std::string line;
                bool firstLine = true;
                while (std::getline(in, line)) {
                    if (firstLine) { firstLine = false; continue; }
                    if (line.empty()) continue;

                    std::stringstream ss(line);
                    std::string csvId, mmr, rk, rkScore;
                    std::getline(ss, csvId, ',');
                    std::getline(ss, mmr, ',');
                    std::getline(ss, rk, ',');
                    std::getline(ss, rkScore, ',');

                    if (csvId == id) {
                        rank = rk;
                        rankScore = rkScore;
                        break;
                    }
                }
                in.close();
            }
        }

        // 4) RankTable.csv → RANK_MIN, RANK_MAX
        std::string rankMin, rankMax;
        {
            std::ifstream in("RankTable.csv");
            if (in.is_open()) {
                std::string line;
                bool firstLine = true;
                while (std::getline(in, line)) {
                    if (firstLine) { firstLine = false; continue; }
                    if (line.empty()) continue;

                    std::stringstream ss(line);
                    std::string rkId, rkName, min, max, extra;
                    std::getline(ss, rkId, ',');
                    std::getline(ss, rkName, ',');
                    std::getline(ss, min, ',');
                    std::getline(ss, max, ',');
                    std::getline(ss, extra, ',');

                    if (rkId == rank) {
                        rankMin = min;
                        rankMax = max;
                        break;
                    }
                }
                in.close();
            }
        }

        // 5) 응답 조합
        if (!nickname.empty() && !level.empty() &&
            !rank.empty() && !rankScore.empty() && !rankMin.empty() && !rankMax.empty()) {

            // 응답 형식: "닉네임,레벨,랭크,랭크점수,랭크MIN,랭크MAX"
            std::string profileData =
                nickname + "," + level + "," + rank + "," + rankScore + "," + rankMin + "," + rankMax;

            sendPacket(client.socket, PacketType::ResponseProfile, profileData);
            std::cout << "[프로필 응답] ID=" << id
                << " 닉네임=" << nickname
                << " 레벨=" << level
                << " 랭크=" << rank
                << " 점수=" << rankScore
                << " Min=" << rankMin
                << " Max=" << rankMax
                << std::endl;
        }
        else {
            sendPacket(client.socket, -1, "프로필 로드 실패");
            std::cout << "[프로필 로드 실패] ID=" << id << std::endl;
        }
        break;
    }
    case PacketType::RequestUserMagic:
    {
        std::cout << "[UserMagic 요청] " << rawMsg << std::endl;

        std::ifstream in("UserMagic.csv");
        if (!in.is_open()) {
            std::cerr << "[서버] UserMagic.csv 열기 실패" << std::endl;
            sendPacket(client.socket, PacketType::ResponseUserMagic, "Error:FileNotFound");
            break;
        }

        std::string line;
        bool firstLine = true;
        bool found = false;

        while (std::getline(in, line)) {
            if (firstLine) { firstLine = false; continue; }
            if (line.empty()) continue;

            std::stringstream ss(line);
            std::string id, m0, m1, m2, m3;
            std::getline(ss, id, ',');
            std::getline(ss, m0, ',');
            std::getline(ss, m1, ',');
            std::getline(ss, m2, ',');
            std::getline(ss, m3, ',');

            if (id == rawMsg) {
                std::string payload = m0 + "," + m1 + "," + m2 + "," + m3;
                sendPacket(client.socket, PacketType::ResponseUserMagic, payload);
                std::cout << "[서버→클라] UserMagic 전송: " << id << " → " << payload << std::endl;
                found = true;
                break;
            }
        }

        if (!found) {
            sendPacket(client.socket, PacketType::ResponseUserMagic, "Error:UserNotFound");
            std::cout << "[서버] UserMagic 데이터 없음: " << rawMsg << std::endl;
        }

        in.close();
        break;
    }
    case PacketType::RequestSetUserMagic:
    {
        std::cout << "[마법 상세 요청] " << rawMsg << std::endl;

        std::string requestedId = rawMsg; // 클라이언트가 보낸 카드 ID (문자열)

        std::ifstream in("CardTable.csv");
        if (!in.is_open())
        {
            std::cerr << "[에러] CardTable.csv 열기 실패" << std::endl;
            PacketManager::sendPacket(client.socket, PacketType::ResponseSetUserMagic, "Error: Cannot open CardTable.csv");
            break;
        }

        std::string line;
        bool firstLine = true;
        bool found = false;

        while (std::getline(in, line))
        {
            if (firstLine) // 첫 번째 라인(헤더)은 건너뜀
            {
                firstLine = false;
                continue;
            }

            if (line.empty()) continue;

            std::stringstream ss(line);
            std::string id;
            std::getline(ss, id, ','); // ID 컬럼 추출

            if (id == requestedId)
            {
                // 요청한 ID와 일치하는 카드 발견 → 그대로 클라이언트로 전송
                PacketManager::sendPacket(client.socket, PacketType::ResponseSetUserMagic, line);
                std::cout << "[서버→클라 전송] 카드 상세: " << line << std::endl;
                found = true;
                break;
            }
        }

        in.close();

        if (!found)
        {
            std::cout << "[경고] CardTable.csv에서 ID=" << requestedId << " 를 찾지 못함" << std::endl;
            PacketManager::sendPacket(client.socket, PacketType::ResponseSetUserMagic, "Error: Card not found");
        }

        break;
    }
    case PacketType::RequestReplaceUserMagic:
    {
        std::cout << "[마법 교체 요청] " << rawMsg << std::endl;

        // msg 예시: "qwe,1,2" (ID, OldCardID, NewCardID)
        std::stringstream ss(rawMsg);
        std::string userId, oldCardStr, newCardStr;
        std::getline(ss, userId, ',');
        std::getline(ss, oldCardStr, ',');
        std::getline(ss, newCardStr, ',');

        if (userId.empty() || oldCardStr.empty() || newCardStr.empty()) {
            break;
        }

        int oldCardId = std::stoi(oldCardStr);
        int newCardId = std::stoi(newCardStr);

        std::ifstream in("UserMagic.csv");
        if (!in.is_open()) {
            std::cerr << "[파일 없음] UserMagic.csv" << std::endl;
            break;
        }

        std::vector<std::string> lines;
        std::string line;
        bool updated = false;

        while (std::getline(in, line)) {
            if (line.empty()) continue;

            // 헤더는 그냥 통과
            if (line.find("ID,") == 0) {
                lines.push_back(line);
                continue;
            }

            std::stringstream ssLine(line);
            std::string id;
            std::getline(ssLine, id, ',');

            if (id == userId) {
                std::vector<std::string> tokens;
                tokens.push_back(id);

                std::string token;
                while (std::getline(ssLine, token, ',')) {
                    tokens.push_back(token);
                }

                // 슬롯 0~3 중 oldCardId를 가진 칸 찾아서 교체
                for (size_t i = 1; i < tokens.size(); i++) {
                    if (tokens[i] == std::to_string(oldCardId)) {
                        std::cout << "[변경 전] 슬롯 " << (i - 1) << " = " << tokens[i] << std::endl;
                        tokens[i] = std::to_string(newCardId);
                        updated = true;
                        std::cout << "[변경 후] 슬롯 " << (i - 1) << " = " << tokens[i] << std::endl;
                        break;
                    }
                }

                // 다시 CSV 라인으로 합침
                std::string newLine;
                for (size_t i = 0; i < tokens.size(); i++) {
                    newLine += tokens[i];
                    if (i != tokens.size() - 1) newLine += ",";
                }
                lines.push_back(newLine);
            }
            else {
                lines.push_back(line);
            }
        }
        in.close();

        // 파일 다시 저장
        if (updated) {
            std::ofstream out("UserMagic.csv", std::ios::trunc);
            for (auto& l : lines) out << l << "\n";
            out.close();

            PacketManager::sendPacket(client.socket, PacketType::ResponseReplaceUserMagic, "마법 교체 성공");
        }

        break;
    }

    default:
        std::cout << "[알 수 없는 패킷] " << packetId << std::endl;
        sendPacket(client.socket, -1, "알 수 없는 패킷");
        break;
    }
}
