#pragma once
#include <string>
#include <vector>
#include <map>
#include "LocationConfig.hpp"

class ServerConfig {
private:
    std::string host;
    std::string serverName;
    int port;
    std::map<int, std::string> errorPages;
    size_t clientMaxBodySize;
    std::vector<LocationConfig> locations;
    
public:
    ServerConfig();
    
    // Getters
    const std::string& getHost() const;
    const std::string& getServerName() const;
    int getPort() const;
    const std::map<int, std::string>& getErrorPages() const;
    size_t getClientMaxBodySize() const;
    const std::vector<LocationConfig>& getLocations() const;
    
    // Setters
    void setHost(const std::string& h);
    void setServerName(const std::string& name);
    void setPort(int p);
    void addErrorPage(int code, const std::string& page);
    void setClientMaxBodySize(size_t size);
    void addLocation(const LocationConfig& location);
};