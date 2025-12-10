#pragma once
#include <string>

class PasswordHasher
{
public:
    static std::string GenerateSalt(size_t length = 16);
    static std::string Hash(const std::string& password, const std::string& salt);
};
