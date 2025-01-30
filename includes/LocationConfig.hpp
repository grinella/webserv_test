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

    std::string redirect;  // Nuovo campo per la redirezione
    int redirectCode;      // Codice HTTP per la redirezione (301 permanent o 302 temporary)
    
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

    const std::string& getRedirect() const { return redirect; }
    int getRedirectCode() const { return redirectCode; }
    bool hasRedirect() const { return !redirect.empty(); }
    
    // Setters
    void setPath(const std::string& p);
    void addAllowedMethod(const std::string& method);
    void setAutoindex(bool ai);
    void setRoot(const std::string& r);
    void setIndex(const std::string& i);
    void addCgiExtension(const std::string& ext);
    void addCgiPath(const std::string& path);

    void setRedirect(const std::string& url, int code = 301) {
        redirect = url;
        redirectCode = code;
    }
};