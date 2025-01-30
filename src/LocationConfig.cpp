#include "../includes/LocationConfig.hpp"

LocationConfig::LocationConfig(const std::string& locationPath) : 
    path(locationPath), 
    autoindex(false),
    redirect(""),
    redirectCode(301) {}

// Getters
const std::string& LocationConfig::getPath() const { return path; }
const std::vector<std::string>& LocationConfig::getAllowedMethods() const { return allowedMethods; }
bool LocationConfig::getAutoindex() const { return autoindex; }
const std::string& LocationConfig::getRoot() const { return root; }
const std::string& LocationConfig::getIndex() const { return index; }
const std::vector<std::string>& LocationConfig::getCgiExtensions() const { return cgiExtensions; }
const std::vector<std::string>& LocationConfig::getCgiPaths() const { return cgiPaths; }

// Setters
void LocationConfig::setPath(const std::string& p) { path = p; }
void LocationConfig::addAllowedMethod(const std::string& method) { allowedMethods.push_back(method); }
void LocationConfig::setAutoindex(bool ai) { autoindex = ai; }
void LocationConfig::setRoot(const std::string& r) { root = r; }
void LocationConfig::setIndex(const std::string& i) { index = i; }
void LocationConfig::addCgiExtension(const std::string& ext) { cgiExtensions.push_back(ext); }
void LocationConfig::addCgiPath(const std::string& path) { cgiPaths.push_back(path); }