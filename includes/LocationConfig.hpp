#pragma once
#include <string>
#include <vector>

class LocationConfig {
private:
    std::string path;
    std::vector<std::string> allowedMethods;
    bool autoindex;
    std::string root;
    std::string index;
    std::vector<std::string> cgiExtensions;
    std::vector<std::string> cgiPaths;
    
public:
    LocationConfig(const std::string& locationPath);
    
    // Getters
    const std::string& getPath() const;
    const std::vector<std::string>& getAllowedMethods() const;
    bool getAutoindex() const;
    const std::string& getRoot() const;
    const std::string& getIndex() const;
    const std::vector<std::string>& getCgiExtensions() const;
    const std::vector<std::string>& getCgiPaths() const;
    
    // Setters
    void setPath(const std::string& p);
    void addAllowedMethod(const std::string& method);
    void setAutoindex(bool ai);
    void setRoot(const std::string& r);
    void setIndex(const std::string& i);
    void addCgiExtension(const std::string& ext);
    void addCgiPath(const std::string& path);
};