#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

inline std::string Trim(const std::string& str) {
    const char* whitespace = " \t\n\r\f\v\r\n";
    size_t start = str.find_first_not_of(whitespace);
    size_t end = str.find_last_not_of(whitespace);
    return (start == std::string::npos) ? str : str.substr(start, end - start + 1);
}

inline std::vector<std::string> SplitCSV(const std::string& line)
{
    std::vector<std::string> result;
    std::stringstream ss(line);
    std::string token;

    while (std::getline(ss, token, ','))
        result.push_back(token);

    return result;
}