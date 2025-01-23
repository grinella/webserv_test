#include "../includes/Utils.hpp"
#include <algorithm>
#include <sstream>

namespace Utils {
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
            return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, last - first + 1);
    }

    size_t findMatchingBrace(const std::string& content, size_t start) {
        int count = 1;
        for (size_t i = start + 1; i < content.length(); ++i) {
            if (content[i] == '{')
                ++count;
            else if (content[i] == '}')
                --count;
            if (count == 0)
                return i;
        }
        return std::string::npos;
    }

    std::vector<std::string> split(const std::string& str, char delim) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delim)) {
            if (!token.empty())
                tokens.push_back(trim(token));
        }
        return tokens;
    }

    bool isValidPort(const std::string& port) {
        if (port.empty()) return false;
        for (size_t i = 0; i < port.length(); ++i) {
            if (!std::isdigit(port[i]))
                return false;
        }
        int portNum = std::atoi(port.c_str());
        return portNum > 0 && portNum <= 65535;
    }

    bool isValidIPAddress(const std::string& ip) {
    std::vector<std::string> parts = split(ip, '.');
    if (parts.size() != 4) return false;
    
    for (size_t i = 0; i < parts.size(); ++i) {
        const std::string& part = parts[i];
        if (part.empty() || part.length() > 3) return false;
        
        for (size_t j = 0; j < part.length(); ++j) {
            if (!std::isdigit(part[j])) return false;
        }
        
        int num = std::atoi(part.c_str());
        if (num < 0 || num > 255) return false;
    }
    return true;
}

    bool isValidSize(const std::string& size) {
        if (size.empty()) return false;
        for (size_t i = 0; i < size.length(); ++i) {
            if (!std::isdigit(size[i]))
                return false;
        }
        return true;
    }

    size_t parseSize(const std::string& size) {
        return std::atoi(size.c_str());
    }

    std::string getDirectiveValue(const std::string& line, const std::string& directive) {
    size_t pos = line.find(directive);
    if (pos == std::string::npos) return "";
    
    std::string value = line.substr(pos + directive.length());
    value = trim(value);
    if (value.empty()) return "";
    
    // Rimuovi il punto e virgola se presente alla fine
    if (!value.empty() && value[value.length() - 1] == ';') {
        value = value.substr(0, value.length() - 1);
    }
    return trim(value);
    }
}