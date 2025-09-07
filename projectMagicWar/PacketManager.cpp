#include "PacketManager.h"
#include "UserAccountManager.h"
#include "UserDataManager.h"
#include "MatchManager.h"
#include "Utils.h"
#include <fstream>   // ifstream, ofstream
#include <sstream>   // stringstream
#include <iostream>

std::unordered_map<std::string, Client>* PacketManager::s_clients = nullptr;
MatchManager* PacketManager::s_match = nullptr;

void PacketManager::Init(std::unordered_map<std::string, Client>* clients, MatchManager* mm) {
    s_clients = clients;
    s_match = mm;
}

void PacketManager::sendPacket(SOCKET sock, int packetId, const std::string& msg) {
    char header[sizeof(int)];
    memcpy(header, &packetId, sizeof(int));

    std::string packet(header, sizeof(int));
    packet += msg;

    int sent = send(sock, packet.c_str(), (int)packet.size(), 0);
    if (sent == SOCKET_ERROR) {
        std::cerr << "[sendPacket 오류] send 실패 (code=" << WSAGetLastError() << ")" << std::endl;
        return;
    }

    std::cout << "[서버→클라] ID=" << packetId
        << " 데이터=" << msg << std::endl;
}

void PacketManager::handlePacket(const Client& client, int packetId, const std::string& rawMsg) {
    std::cout << "수신: ID=" << packetId << " 데이터=" << rawMsg << std::endl;

    switch (packetId) {
    case PacketType::Signup: {
        std::cout << "[회원가입 요청] " << rawMsg << std::endl;

        // rawMsg 형식: "id,password,nickname"
        size_t first = rawMsg.find(',');
        size_t second = rawMsg.find(',', first + 1);
        if (first == std::string::npos || second == std::string::npos) {
            sendPacket(client.socket, PacketType::SignupIDError, "잘못된 요청");
            break;
        }

        std::string id = rawMsg.substr(0, first);
        std::string pw = rawMsg.substr(first + 1, second - first - 1);
        std::string nickname = rawMsg.substr(second + 1);

        UserAccountManager mgr("UserAccount.csv");
        UserAccount newAcc{ id, pw, nickname };

        int resultType = PacketType::SignupSuccess;
        if (mgr.addAccount(newAcc, resultType)) {
            // 가입 성공 → UserData.csv 등 생성
            UserDataManager::createUserData(id);

            sendPacket(client.socket, PacketType::SignupSuccess, id);
            std::cout << "[회원가입 성공] ID=" << id << " NickName=" << nickname << std::endl;
        }
        else {
            if (resultType == PacketType::SignupIDError) {
                sendPacket(client.socket, PacketType::SignupIDError, id);
                std::cout << "[회원가입 실패] 이미 존재하는 ID=" << id << std::endl;
            }
            else if (resultType == PacketType::SignupNickError) {
                sendPacket(client.socket, PacketType::SignupNickError, nickname);
                std::cout << "[회원가입 실패] 이미 존재하는 NickName=" << nickname << std::endl;
            }
            else {
                sendPacket(client.socket, -1, "Internal Error");
                std::cout << "[회원가입 실패] 서버 내부 오류" << std::endl;
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

                    if (s_clients) {
                        (*s_clients)[id] = client;
                        std::cout << "[클라이언트 등록] userId=" << id
                            << " socket=" << client.socket << std::endl;
                    }
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
    case PacketType::RequestMatching:
    {
        std::string userId = Trim(rawMsg);
        s_match->HandleRequestMatching(userId);
        break;
    }
    case PacketType::CancelMatch:
    {
        std::string userId = Trim(rawMsg);
        if (!userId.empty()) {
            s_match->HandleCancelMatch(userId);
        }
        break;
    }
    case PacketType::RequestMagicCooldowns:
    {
        std::string userId = Trim(rawMsg);
        std::cout << "[RequestMagicCooldowns] userId=" << userId << std::endl;

        // 1) UserMagic.csv 에서 유저의 4개 슬롯 읽기
        std::vector<int> magicIds;
        {
            std::ifstream in("UserMagic.csv");
            if (!in.is_open()) {
                std::cerr << "[에러] UserMagic.csv 열기 실패" << std::endl;
                sendPacket(client.socket, PacketType::ResponseUserMagicCooldowns, "Error:FileNotFound");
                break;
            }

            std::string line;
            bool firstLine = true;
            while (std::getline(in, line)) {
                if (firstLine) { firstLine = false; continue; }
                if (line.empty()) continue;

                std::stringstream ss(line);
                std::string csvId, m0, m1, m2, m3;
                std::getline(ss, csvId, ',');
                std::getline(ss, m0, ',');
                std::getline(ss, m1, ',');
                std::getline(ss, m2, ',');
                std::getline(ss, m3, ',');

                if (csvId == userId) {
                    if (!m0.empty()) magicIds.push_back(std::stoi(m0));
                    if (!m1.empty()) magicIds.push_back(std::stoi(m1));
                    if (!m2.empty()) magicIds.push_back(std::stoi(m2));
                    if (!m3.empty()) magicIds.push_back(std::stoi(m3));
                    break;
                }
            }
            in.close();
        }

        if (magicIds.empty()) {
            std::cerr << "[경고] 유저 " << userId << " 의 마법 슬롯이 비어있음" << std::endl;
            sendPacket(client.socket, PacketType::ResponseUserMagicCooldowns, "Error:NoMagic");
            break;
        }

        // 2) CardTable.csv 에서 각 카드의 속성 찾기
        // CSV 포맷: ID,Name,Attack,Cooldown,Cost,EffectType,Speed,Radius,Duration,Amount
        std::unordered_map<int, std::string> cardInfo;
        {
            std::ifstream in("CardTable.csv");
            if (!in.is_open()) {
                std::cerr << "[에러] CardTable.csv 열기 실패" << std::endl;
                sendPacket(client.socket, PacketType::ResponseUserMagicCooldowns, "Error:CardTableNotFound");
                break;
            }

            std::string line;
            bool firstLine = true;
            while (std::getline(in, line)) {
                if (firstLine) { firstLine = false; continue; }
                if (line.empty()) continue;

                std::stringstream ss(line);
                std::string idStr, name, attack, cooldown, cost, effect, speed, radius, duration, amount;
                std::getline(ss, idStr, ',');
                std::getline(ss, name, ',');
                std::getline(ss, attack, ',');
                std::getline(ss, cooldown, ',');
                std::getline(ss, cost, ',');
                std::getline(ss, effect, ',');
                std::getline(ss, speed, ',');
                std::getline(ss, radius, ',');
                std::getline(ss, duration, ',');
                std::getline(ss, amount, ',');

                int id = std::stoi(idStr);
                // 클라에서 원하는 포맷: "ID:Cooldown:Attack:Speed:Radius:Duration:Amount"
                std::string formatted = idStr + ":" + cooldown + ":" + attack + ":" +
                    speed + ":" + radius + ":" + duration + ":" + amount;
                cardInfo[id] = formatted;
            }
            in.close();
        }

        // 3) 유저의 슬롯 순서대로 응답 문자열 생성
        std::string response;
        for (size_t i = 0; i < magicIds.size(); i++) {
            int id = magicIds[i];
            if (cardInfo.find(id) != cardInfo.end()) {
                if (!response.empty()) response += ",";
                response += cardInfo[id];
            }
        }

        if (response.empty()) {
            response = "Error:NoCardData";
        }

        // 4) 클라이언트에 전송
        sendPacket(client.socket, PacketType::ResponseUserMagicCooldowns, response);
        std::cout << "[ResponseUserMagicCooldowns] userId=" << userId
            << " → " << response << std::endl;

        break;
    }
    case PacketType::Move:
    {
        //std::cout << "[Move] 데이터=" << rawMsg << std::endl;

        // msg 예: "qwe,2.35,-1.20"
        std::stringstream ss(rawMsg);
        std::string userId, xStr, yStr;
        std::getline(ss, userId, ',');
        std::getline(ss, xStr, ',');
        std::getline(ss, yStr, ',');

        if (userId.empty() || xStr.empty() || yStr.empty())
        {
            std::cerr << "[Move] 잘못된 데이터: " << rawMsg << std::endl;
            break;
        }

        float x = std::stof(xStr);
        float y = std::stof(yStr);

        // 다른 클라이언트에게만 전송
        for (auto& kv : *s_clients)
        {
            const std::string& targetId = kv.first;
            SOCKET sock = kv.second.socket;

            if (targetId == userId)
                continue; // 자기 자신은 스킵

            // 상대 클라에게는 y 반전
            float sendX = x;
            float sendY = -y;

            std::string payload = userId + "," + std::to_string(sendX) + "," + std::to_string(sendY);
            sendPacket(sock, PacketType::Move, payload);

            std::cout << "[서버→클라] 대상=" << targetId
                << " 상대=" << userId
                << " 이동=" << payload << std::endl;
        }

        break;
    }
    case PacketType::CastMagic:
    {
        // msg: "userId,cardId,x,y,dirX,dirY"
        std::stringstream ss(rawMsg);
        std::string userId, cardIdStr, xStr, yStr, dirXStr, dirYStr;
        std::getline(ss, userId, ',');
        std::getline(ss, cardIdStr, ',');
        std::getline(ss, xStr, ',');
        std::getline(ss, yStr, ',');
        std::getline(ss, dirXStr, ',');
        std::getline(ss, dirYStr, ',');

        int cardId = std::stoi(cardIdStr);
        float x = std::stof(xStr);
        float y = std::stof(yStr);
        float dirX = std::stof(dirXStr);
        float dirY = std::stof(dirYStr);

        // --- CardTable.csv 에서 해당 카드 속성 읽기 ---
        int attack = 0;
        float speed = 0.0f;
        {
            std::ifstream in("CardTable.csv");
            if (in.is_open())
            {
                std::string line;
                bool firstLine = true;
                while (std::getline(in, line))
                {
                    if (firstLine) { firstLine = false; continue; }
                    if (line.empty()) continue;

                    std::stringstream ss(line);
                    std::string idStr, name, attackStr, cooldown, cost, effect, speedStr, radius, duration, amount;
                    std::getline(ss, idStr, ',');
                    std::getline(ss, name, ',');
                    std::getline(ss, attackStr, ',');
                    std::getline(ss, cooldown, ',');
                    std::getline(ss, cost, ',');
                    std::getline(ss, effect, ',');
                    std::getline(ss, speedStr, ',');
                    std::getline(ss, radius, ',');
                    std::getline(ss, duration, ',');
                    std::getline(ss, amount, ',');

                    if (std::stoi(idStr) == cardId)
                    {
                        attack = std::stoi(attackStr);
                        speed = std::stof(speedStr);
                        break;
                    }
                }
                in.close();
            }
        }

        // --- 다른 클라들에게 전달 ---
        for (auto& kv : *s_clients)
        {
            if (kv.first == userId) continue;

            SOCKET sock = kv.second.socket;

            float sendX = x;
            float sendY = -y;
            float sendDirX = dirX;
            float sendDirY = -dirY;

            std::string payload =
                userId + "," + cardIdStr + "," +
                std::to_string(sendX) + "," +
                std::to_string(sendY) + "," +
                std::to_string(sendDirX) + "," +
                std::to_string(sendDirY) + "," +
                std::to_string(attack) + "," +
                std::to_string(speed);

            sendPacket(sock, PacketType::SpawnMagic, payload);

            /*std::cout << "[서버→클라] SpawnMagic 대상=" << kv.first
                << " 상대=" << userId
                << " 데이터=" << payload << std::endl;*/
        }

        break;
    }
    case PacketType::Hit:
    {
        // msg: "targetUserId,damage"
        std::stringstream ss(rawMsg);
        std::string targetId, damageStr;
        std::getline(ss, targetId, ',');
        std::getline(ss, damageStr, ',');

        if (targetId.empty() || damageStr.empty()) {
            std::cerr << "[Hit] 잘못된 데이터: " << rawMsg << std::endl;
            break;
        }

        int damage = std::stoi(damageStr);

        // 브로드캐스트 (맞은 유저 ID, 데미지)
        std::string payload = targetId + "," + std::to_string(damage);
        for (auto& kv : *s_clients)
        {
            sendPacket(kv.second.socket, PacketType::DamageResult, payload);
        }

        std::cout << "[Hit] target=" << targetId
            << " damage=" << damage << std::endl;
        break;
    }
    case PacketType::Die:
    {
        // msg: "userId"
        std::string deadUserId = rawMsg;

        std::string opponentId;
        for (auto& kv : *s_clients)
        {
            if (kv.first != deadUserId)
            {
                opponentId = kv.first;
                break;
            }
        }

        // 죽은 유저 → 패배
        if (s_clients->count(deadUserId))
        {
            sendPacket((*s_clients)[deadUserId].socket, PacketType::Defeat, "LOSE");
        }

        // 상대 유저 → 승리
        if (s_clients->count(opponentId))
        {
            sendPacket((*s_clients)[opponentId].socket, PacketType::Victory, "WIN");
        }

        std::cout << "[Game] " << deadUserId << " 패배, " << opponentId << " 승리" << std::endl;
        break;
    }

    default:
        std::cout << "[알 수 없는 패킷] " << packetId << std::endl;
        sendPacket(client.socket, -1, "알 수 없는 패킷");
        break;
    }
}
