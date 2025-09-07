#pragma once
#include <string>
#include <algorithm>

inline std::string Trim(const std::string& str) {
    const char* whitespace = " \t\n\r\f\v\r\n";
    size_t start = str.find_first_not_of(whitespace);
    size_t end = str.find_last_not_of(whitespace);
    return (start == std::string::npos) ? str : str.substr(start, end - start + 1);
}
