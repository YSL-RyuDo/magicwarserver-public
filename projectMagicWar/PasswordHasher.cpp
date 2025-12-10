#include "PasswordHasher.h"
#include "../Util/SHA256.h"
#include <random>

std::string PasswordHasher::GenerateSalt(size_t length)
{
    static const char chars[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!@#$%^&*()";

    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());
    thread_local std::uniform_int_distribution<> dist(0, sizeof(chars) - 2);

    std::string salt;
    salt.reserve(length);

    for (size_t i = 0; i < length; ++i)
        salt.push_back(chars[dist(gen)]);

    return salt;
}

std::string PasswordHasher::Hash(const std::string& password, const std::string& salt)
{
    SHA256 sha;

    // 1Â÷ ÇØ½Ã
    std::string result = sha.CalculateHex(password + salt);

    // ½ºÆ®·¹Äª 999¹ø (ÃÑ 1000È¸)
    for (int i = 0; i < 999; i++)
        result = sha.CalculateHex(result + salt);

    return result;
}
