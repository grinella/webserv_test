#include "../includes/ServerConfig.hpp"

ServerConfig::ServerConfig() : port(0), clientMaxBodySize(0) {}

// Getters
const std::string& ServerConfig::getHost() const { return host; }
const std::string& ServerConfig::getServerName() const { return serverName; }
int ServerConfig::getPort() const { return port; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return errorPages; }
size_t ServerConfig::getClientMaxBodySize() const { return clientMaxBodySize; }
const std::vector<LocationConfig>& ServerConfig::getLocations() const { return locations; }

// Setters
void ServerConfig::setHost(const std::string& h) { host = h; }
void ServerConfig::setServerName(const std::string& name) { serverName = name; }
void ServerConfig::setPort(int p) { port = p; }
void ServerConfig::addErrorPage(int code, const std::string& page) { errorPages[code] = page; }
void ServerConfig::setClientMaxBodySize(size_t size) { clientMaxBodySize = size; }
void ServerConfig::addLocation(const LocationConfig& location) { locations.push_back(location); }