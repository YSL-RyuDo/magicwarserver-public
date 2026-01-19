#include "MagicManager.h"
#include <fstream>
#include <sstream>

std::mutex MagicManager::s_mtx;
std::string MagicManager::s_magicInfoFile;
std::string MagicManager::s_userMagicFile;
std::unordered_map<int, MagicInfo> MagicManager::s_magicTable;

void MagicManager::Init(const std::string& magicInfoFile,
    const std::string& userMagicFile)
{
    s_magicInfoFile = magicInfoFile;
    s_userMagicFile = userMagicFile;

    std::ifstream fin(magicInfoFile);
    std::string line;

    // Çì´õ ½ºÅµ
    std::getline(fin, line);

    while (std::getline(fin, line))
    {
        std::stringstream ss(line);
        std::string tok;

        MagicInfo info;

        std::getline(ss, tok, ','); info.id = std::stoi(tok);
        std::getline(ss, info.name, ',');
        std::getline(ss, tok, ','); info.attack = std::stoi(tok);
        std::getline(ss, tok, ','); info.cooldown = std::stof(tok);
        std::getline(ss, tok, ','); info.cost = std::stoi(tok);
        std::getline(ss, info.effectType, ',');
        std::getline(ss, tok, ','); info.speed = std::stof(tok);
        std::getline(ss, tok, ','); info.radius = std::stof(tok);
        std::getline(ss, tok, ','); info.duration = std::stof(tok);
        std::getline(ss, tok, ','); info.amount = std::stoi(tok);

        s_magicTable[info.id] = info;
    }
}


bool MagicManager::LoadUserMagic(const std::string& nick, std::vector<int>& outSlots)
{
    std::lock_guard<std::mutex> lock(s_mtx);

    std::ifstream fin(s_userMagicFile);
    std::string line;

    while (std::getline(fin, line))
    {
        std::stringstream ss(line);
        std::string tok;

        std::getline(ss, tok, ',');
        if (tok != nick) continue;

        outSlots.clear();

        for (int i = 0; i < 4; i++)
        {
            std::getline(ss, tok, ',');
            outSlots.push_back(std::stoi(tok));
        }
        return true;
    }
    return false;
}

bool MagicManager::ReplaceMagic(const std::string& nick,
    int slotIndex,
    int magicId)
{
    std::lock_guard<std::mutex> lock(s_mtx);

    std::ifstream fin(s_userMagicFile);
    std::vector<std::string> lines;
    std::string line;

    while (std::getline(fin, line))
    {
        std::stringstream ss(line);
        std::string name;
        std::getline(ss, name, ',');

        if (name == nick)
        {
            std::vector<int> slots;
            std::string tok;

            for (int i = 0; i < 4; i++)
            {
                std::getline(ss, tok, ',');

                if (i == slotIndex)
                    slots.push_back(magicId);
                else
                    slots.push_back(std::stoi(tok));
            }

            std::string newLine = name;
            for (int x : slots)
                newLine += "," + std::to_string(x);

            lines.push_back(newLine);
        }
        else
        {
            lines.push_back(line);
        }
    }
    fin.close();

    std::ofstream fout(s_userMagicFile, std::ios::trunc);
    for (auto& l : lines)
        fout << l << "\n";

    return true;
}

bool MagicManager::GetMagicInfo(int id, MagicInfo& out)
{
    std::lock_guard<std::mutex> lock(s_mtx);

    auto it = s_magicTable.find(id);
    if (it == s_magicTable.end())
        return false;

    out = it->second;
    return true;
}
