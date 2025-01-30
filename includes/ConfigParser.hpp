#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include "Utils.hpp"

#ifndef F_OK
#define F_OK 0
#endif

#ifndef X_OK
#define X_OK 1
#endif

class ConfigParser {
private:
    std::string configFile;
    std::vector<ServerConfig> servers;
    std::map<std::string, bool> seenDirectives;

    // Core parsing methods
    void parseFile();
    ServerConfig parseServer(const std::string& serverBlock);
    void parseLocation(const std::string& locationBlock, LocationConfig& location);

    // Server directive parsers
    void parseServerDirective(const std::string& line, ServerConfig& server);
    void parseHost(const std::string& line, ServerConfig& server);
    void parsePort(const std::string& line, ServerConfig& server);
    void parseServerName(const std::string& line, ServerConfig& server);
    void parseErrorPage(const std::string& line, ServerConfig& server);
    void parseClientMaxBodySize(const std::string& line, ServerConfig& server);

    // Location directive parsers
    void parseLocationDirective(const std::string& line, LocationConfig& location);
    void parseMethods(const std::string& line, LocationConfig& location);
    void parseAutoindex(const std::string& line, LocationConfig& location);
    void parseRoot(const std::string& line, LocationConfig& location);
    void parseIndex(const std::string& line, LocationConfig& location);
    void parseCgiExt(const std::string& line, LocationConfig& location);
    void parseCgiPath(const std::string& line, LocationConfig& location);
    void parseRedirect(const std::string& line, LocationConfig& location);

    // Validation methods
    bool validateSyntax(const std::string& content);
    bool validateServer(const ServerConfig& server);
    bool validateLocation(const LocationConfig& location);

public:
    ConfigParser(const std::string& filename);
    ~ConfigParser();

    const std::vector<ServerConfig>& getServers() const;
};