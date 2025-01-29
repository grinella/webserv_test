#pragma once
#include <string>
#include <vector>

namespace Utils {
    std::string trim(const std::string& str);
    size_t findMatchingBrace(const std::string& content, size_t start);
    std::vector<std::string> split(const std::string& str, char delim);
    bool isValidPort(const std::string& port);
    bool isValidPath(const std::string& path);
    bool isValidIPAddress(const std::string& ip);
    bool isValidSize(const std::string& size);
    size_t parseSize(const std::string& size);
    std::string getDirectiveValue(const std::string& line, const std::string& directive);
    std::string U_intToString(int value);
}