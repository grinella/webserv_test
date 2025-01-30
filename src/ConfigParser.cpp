#include "../includes/ConfigParser.hpp"
#include "../includes/Utils.hpp"
// include for std::atoi
#include <cstdlib>

ConfigParser::ConfigParser(const std::string& filename) : configFile(filename) {
    parseFile();
}

ConfigParser::~ConfigParser() {}

const std::vector<ServerConfig>& ConfigParser::getServers() const {
    return servers;
}

bool ConfigParser::validateLocation(const LocationConfig& location) {
    static std::map<std::string, bool> seenDirectives;
    seenDirectives.clear();

    // Check for empty path
    if (location.getPath().empty()) {
        return false;
    }

    // Check for duplicate methods
    const std::vector<std::string>& methods = location.getAllowedMethods();
    std::map<std::string, bool> seenMethods;
    for (size_t i = 0; i < methods.size(); ++i) {
        if (seenMethods[methods[i]]) {
            throw std::runtime_error("Duplicate method in allow_methods: " + methods[i]);
        }
        seenMethods[methods[i]] = true;

        // Verify method is valid
        if (methods[i] != "GET" && methods[i] != "POST" && methods[i] != "DELETE") {
            throw std::runtime_error("Invalid HTTP method: " + methods[i]);
        }
    }

    // Check if CGI paths exist for CGI extensions
    const std::vector<std::string>& cgiExt = location.getCgiExtensions();
    const std::vector<std::string>& cgiPath = location.getCgiPaths();
    if (!cgiExt.empty()) {
        if (cgiPath.empty()) {
            throw std::runtime_error("CGI extensions defined but no CGI paths provided");
        }
        // Verify CGI paths exist
        for (size_t i = 0; i < cgiPath.size(); ++i) {
            if (access(cgiPath[i].c_str(), X_OK) == -1) {
                throw std::runtime_error("CGI interpreter not found or not executable: " + cgiPath[i]);
            }
        }
    }

    return true;
}

void ConfigParser::parseFile() {
    std::ifstream file(configFile.c_str());
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file");
        
    std::string content;
    std::string line;
    while (std::getline(file, line)) {
        // Remove comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line = line.substr(0, commentPos);
        if (!line.empty())
            content += line + "\n";
    }
    
    if (!validateSyntax(content))
        throw std::runtime_error("Invalid config file syntax");

    std::map<std::string, int> serverPortCount;    // Per tenere traccia delle porte per host
    std::map<std::string, bool> locationPaths;     // Per tenere traccia delle location già definite
    
    // Split into server blocks
    size_t pos = 0;
    while ((pos = content.find_first_not_of(" \t\n\r", pos)) != std::string::npos) {
        // Verifica che il blocco inizi con "server"
        if (content.substr(pos, 6) != "server")
            throw std::runtime_error("Invalid configuration: found non-server block");
            
        pos = content.find("{", pos);
        if (pos == std::string::npos)
            throw std::runtime_error("Invalid server block: missing opening brace");
            
        size_t end = Utils::findMatchingBrace(content, pos);
        if (end == std::string::npos)
            throw std::runtime_error("Unclosed server block");
            
        std::string serverBlock = content.substr(pos + 1, end - (pos + 1));
        ServerConfig server = parseServer(serverBlock);
        
        // Verifica duplicati host:port
        std::string hostPort = server.getHost() + ":" + Utils::U_intToString(server.getPort());
        if (serverPortCount[hostPort]++ > 0)
            throw std::runtime_error("Duplicate host:port combination: " + hostPort);
            
        if (!validateServer(server))
            throw std::runtime_error("Invalid server configuration");
            
        servers.push_back(server);
        pos = end + 1;
        
        // Reset locationPaths per il prossimo server
        locationPaths.clear();
    }
    
    if (servers.empty())
        throw std::runtime_error("No valid server configurations found");
}

ServerConfig ConfigParser::parseServer(const std::string& serverBlock) {
    ServerConfig server;
    std::istringstream iss(serverBlock);
    std::string line;
    std::string currentLine;
    
    // Reset della mappa delle direttive
    seenDirectives.clear();
    
    // Mappa per tenere traccia delle location già viste
    std::map<std::string, bool> seenLocations;
    
    while (std::getline(iss, line)) {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#') 
            continue;
        
        if (line.find("location ") == 0) {
            size_t pathStart = 9;
            size_t pathEnd = line.find("{", pathStart);
            if (pathEnd == std::string::npos)
                throw std::runtime_error("Invalid location syntax");
                
            std::string locationPath = Utils::trim(line.substr(pathStart, pathEnd - pathStart));
            if (locationPath.empty())
                throw std::runtime_error("Empty location path");

            // Controlla se questa location è già stata definita
            if (seenLocations[locationPath])
                throw std::runtime_error("Duplicate location definition: " + locationPath);
            seenLocations[locationPath] = true;
                
            size_t start = serverBlock.find(line) + line.length();
            size_t end = Utils::findMatchingBrace(serverBlock, start);
            if (end == std::string::npos)
                throw std::runtime_error("Unclosed location block");
            
            currentLine = line;
            std::string locationBlock = serverBlock.substr(start + 1, end - (start + 1));
            LocationConfig location(locationPath);
            parseLocation(locationBlock, location);
            
            if (!validateLocation(location)) {
                throw std::runtime_error("Invalid location configuration for path: " + locationPath);
            }
            
            server.addLocation(location);
            iss.clear();
            iss.str(serverBlock.substr(end + 1));
        } else if (line != currentLine) {
            parseServerDirective(line, server);
        }
        std::cout << "\nParsed Server Configuration:" << std::endl;
        for (size_t i = 0; i < server.getLocations().size(); ++i) {
            const LocationConfig& loc = server.getLocations()[i];
            std::cout << "Location: " << loc.getPath() << std::endl;
            if (loc.hasRedirect()) {
                std::cout << "  - Redirect to: " << loc.getRedirect() 
                        << " (code: " << loc.getRedirectCode() << ")" << std::endl;
            }
        }
        std::cout << std::endl;
    }

    // Verifica che le direttive obbligatorie siano presenti
    if (!seenDirectives["host"])
        throw std::runtime_error("Missing host directive in server block");
    if (!seenDirectives["port"])
        throw std::runtime_error("Missing port directive in server block");

    // Verifica che ci sia almeno una location
    if (server.getLocations().empty())
        throw std::runtime_error("Server block must contain at least one location");

    return server;
}

void ConfigParser::parseServerDirective(const std::string& line, ServerConfig& server) {
    std::string directive;

    if (line.find("host") == 0) {
        directive = "host";
        if (seenDirectives[directive])
            throw std::runtime_error("Duplicate host directive in server block");
        seenDirectives[directive] = true;
        parseHost(line, server);
    }
    else if (line.find("port") == 0) {
        directive = "port";
        if (seenDirectives[directive])
            throw std::runtime_error("Duplicate port directive in server block");
        seenDirectives[directive] = true;
        parsePort(line, server);
    }
    else if (line.find("server_name") == 0) {
        directive = "server_name";
        if (seenDirectives[directive])
            throw std::runtime_error("Duplicate server_name directive in server block");
        seenDirectives[directive] = true;
        parseServerName(line, server);
    }
    else if (line.find("error_page") == 0) {
        parseErrorPage(line, server);
    }
    else if (line.find("client_max_body_size") == 0) {
        directive = "client_max_body_size";
        if (seenDirectives[directive])
            throw std::runtime_error("Duplicate client_max_body_size directive in server block");
        seenDirectives[directive] = true;
        parseClientMaxBodySize(line, server);
    }
    else
        throw std::runtime_error("Unknown server directive: " + line);
}

// Specific directive parsers
void ConfigParser::parseHost(const std::string& line, ServerConfig& server) {
    std::string host = Utils::getDirectiveValue(line, "host");
    if (!Utils::isValidIPAddress(host))
        throw std::runtime_error("Invalid host address: " + host);
    server.setHost(host);
}

void ConfigParser::parsePort(const std::string& line, ServerConfig& server) {
    std::string portStr = Utils::getDirectiveValue(line, "port");
    if (!Utils::isValidPort(portStr))
        throw std::runtime_error("Invalid port number: " + portStr);
    server.setPort(std::atoi(portStr.c_str()));
}

void ConfigParser::parseServerName(const std::string& line, ServerConfig& server) {
    std::string name = Utils::getDirectiveValue(line, "server_name");
    if (name.empty())
        throw std::runtime_error("Empty server name");
    server.setServerName(name);
}

void ConfigParser::parseErrorPage(const std::string& line, ServerConfig& server) {
    std::vector<std::string> parts = Utils::split(Utils::getDirectiveValue(line, "error_page"), ' ');
    if (parts.size() != 2)
        throw std::runtime_error("Invalid error_page directive");
        
    int errorCode = std::atoi(parts[0].c_str());
    if (errorCode < 100 || errorCode > 599)
        throw std::runtime_error("Invalid error code: " + parts[0]);
        
    server.addErrorPage(errorCode, parts[1]);
}

void ConfigParser::parseClientMaxBodySize(const std::string& line, ServerConfig& server) {
    std::string size = Utils::getDirectiveValue(line, "client_max_body_size");
    if (!Utils::isValidSize(size))
        throw std::runtime_error("Invalid client_max_body_size: " + size);
    server.setClientMaxBodySize(Utils::parseSize(size));
}

// Location parsing
void ConfigParser::parseLocation(const std::string& locationBlock, LocationConfig& location) {
    std::istringstream iss(locationBlock);
    std::string line;
    
    while (std::getline(iss, line)) {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#') continue;
        parseLocationDirective(line, location); // Usa la funzione membro
    }
}

// Validation methods
bool ConfigParser::validateSyntax(const std::string& content) {
    // Basic syntax validation
    int braceCount = 0;
    bool inQuotes = false;
    
    for (size_t i = 0; i < content.length(); ++i) {
        if (content[i] == '"')
            inQuotes = !inQuotes;
        else if (!inQuotes) {
            if (content[i] == '{')
                ++braceCount;
            else if (content[i] == '}')
                --braceCount;
                
            if (braceCount < 0)
                return false;
        }
    }
    
    return braceCount == 0;
}

bool ConfigParser::validateServer(const ServerConfig& server) {
    static std::map<std::string, bool> seenDirectives;
    seenDirectives.clear();

    // Check required directives
    if (server.getHost().empty() || server.getPort() <= 0) {
        return false;
    }

    // Check host format
    std::string host = server.getHost();
    if (host[host.length() - 1] == '.' || !Utils::isValidIPAddress(host)) {
        throw std::runtime_error("Invalid host format: " + host);
    }

    // Check error pages exist
    const std::map<int, std::string>& errorPages = server.getErrorPages();
    for (std::map<int, std::string>::const_iterator it = errorPages.begin(); 
         it != errorPages.end(); ++it) {
        std::string path = it->second;
        if (path[0] == '/') {
            path = "." + path;
        }
        if (access(path.c_str(), F_OK) == -1) {
            throw std::runtime_error("Error page not found: " + it->second);
        }
    }

    // Check locations
    std::map<std::string, bool> locationPaths;
    const std::vector<LocationConfig>& locations = server.getLocations();
    for (size_t i = 0; i < locations.size(); ++i) {
        const LocationConfig& loc = locations[i];
        std::string path = loc.getPath();
        
        if (locationPaths[path]) {
            throw std::runtime_error("Duplicate location path: " + path);
        }
        locationPaths[path] = true;

        if (!validateLocation(loc)) {
            throw std::runtime_error("Invalid location configuration for path: " + path);
        }
    }

    return true;
}

void ConfigParser::parseLocationDirective(const std::string& line, LocationConfig& location) {
    std::cout << "Parsing location directive: " << line << std::endl;  // Debug log
    if (line.find("allow_methods") == 0)
        parseMethods(line, location);
    else if (line.find("autoindex") == 0)
        parseAutoindex(line, location);
    else if (line.find("root") == 0)
        parseRoot(line, location);
    else if (line.find("index") == 0)
        parseIndex(line, location);
    else if (line.find("cgi_ext") == 0)
        parseCgiExt(line, location);
    else if (line.find("cgi_path") == 0)
        parseCgiPath(line, location);
    else if (line.find("return") == 0) {
        std::cout << "Found return directive: " << line << std::endl;  // Debug log
        parseRedirect(line, location);
    }
    else
        throw std::runtime_error("Unknown location directive: " + line);
}

void ConfigParser::parseMethods(const std::string& line, LocationConfig& location) {
    std::string methodsStr = Utils::getDirectiveValue(line, "allow_methods");
    std::vector<std::string> methods = Utils::split(methodsStr, ' ');
    for (size_t i = 0; i < methods.size(); ++i) {
        if (methods[i] != "GET" && methods[i] != "POST" && methods[i] != "DELETE")
            throw std::runtime_error("Invalid HTTP method: " + methods[i]);
        location.addAllowedMethod(methods[i]);
    }
}

void ConfigParser::parseAutoindex(const std::string& line, LocationConfig& location) {
    std::string value = Utils::getDirectiveValue(line, "autoindex");
    value = Utils::trim(value);
    if (value != "on" && value != "off")
        throw std::runtime_error("Invalid autoindex value. Must be 'on' or 'off'");
    location.setAutoindex(value == "on");
}

void ConfigParser::parseRoot(const std::string& line, LocationConfig& location) {
    std::string root = Utils::getDirectiveValue(line, "root");
    if (root.empty())
        throw std::runtime_error("Empty root path");
    location.setRoot(root);
}

void ConfigParser::parseIndex(const std::string& line, LocationConfig& location) {
    std::string index = Utils::getDirectiveValue(line, "index");
    if (index.empty())
        throw std::runtime_error("Empty index value");
    location.setIndex(index);
}

void ConfigParser::parseCgiExt(const std::string& line, LocationConfig& location) {
    std::string extsStr = Utils::getDirectiveValue(line, "cgi_ext");
    std::vector<std::string> exts = Utils::split(extsStr, ' ');
    for (size_t i = 0; i < exts.size(); ++i) {
        location.addCgiExtension(exts[i]);
    }
}

void ConfigParser::parseCgiPath(const std::string& line, LocationConfig& location) {
    std::string pathsStr = Utils::getDirectiveValue(line, "cgi_path");
    std::vector<std::string> paths = Utils::split(pathsStr, ' ');
    for (size_t i = 0; i < paths.size(); ++i) {
        location.addCgiPath(paths[i]);
    }
}

void ConfigParser::parseRedirect(const std::string& line, LocationConfig& location) {
    std::string value = Utils::getDirectiveValue(line, "return");
    if (value.empty())
        throw std::runtime_error("Empty redirect value");

    std::vector<std::string> parts = Utils::split(value, ' ');
    int code;
    std::string url;

    if (parts.size() == 1) {
        // Solo URL, usa 301 come default
        code = 301;
        url = parts[0];
    } else if (parts.size() == 2) {
        // Codice e URL
        code = std::atoi(parts[0].c_str());
        url = parts[1];
        
        // Verifica codice valido
        if (code != 301 && code != 302) {
            throw std::runtime_error("Invalid redirect code. Must be 301 or 302");
        }
    } else {
        throw std::runtime_error("Invalid redirect syntax");
    }

    location.setRedirect(url, code);
}

void ConfigParser::debugPrint() const {
    std::cout << "\n=== Configuration Debug Output ===\n\n";
    
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& server = servers[i];
        std::cout << "SERVER [" << i + 1 << "] {\n";
        std::cout << "    Host: " << server.getHost() << "\n";
        std::cout << "    Port: " << server.getPort() << "\n";
        std::cout << "    Server Name: " << server.getServerName() << "\n";
        std::cout << "    Client Max Body Size: " << server.getClientMaxBodySize() << "\n";
        
        std::cout << "    Error Pages {\n";
        const std::map<int, std::string>& errorPages = server.getErrorPages();
        for (std::map<int, std::string>::const_iterator it = errorPages.begin(); 
             it != errorPages.end(); ++it) {
            std::cout << "        " << it->first << ": " << it->second << "\n";
        }
        std::cout << "    }\n";
        
        const std::vector<LocationConfig>& locations = server.getLocations();
        for (size_t j = 0; j < locations.size(); ++j) {
            const LocationConfig& loc = locations[j];
            std::cout << "    Location [" << loc.getPath() << "] {\n";
            
            std::cout << "        Allowed Methods: ";
            const std::vector<std::string>& methods = loc.getAllowedMethods();
            for (size_t k = 0; k < methods.size(); ++k) {
                std::cout << methods[k];
                if (k < methods.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
            
            std::cout << "        Autoindex: " << (loc.getAutoindex() ? "on" : "off") << "\n";
            std::cout << "        Root: " << loc.getRoot() << "\n";
            std::cout << "        Index: " << loc.getIndex() << "\n";
            
            const std::vector<std::string>& cgiExt = loc.getCgiExtensions();
            if (!cgiExt.empty()) {
                std::cout << "        CGI Extensions: ";
                for (size_t k = 0; k < cgiExt.size(); ++k) {
                    std::cout << cgiExt[k];
                    if (k < cgiExt.size() - 1) std::cout << ", ";
                }
                std::cout << "\n";
            }
            
            const std::vector<std::string>& cgiPaths = loc.getCgiPaths();
            if (!cgiPaths.empty()) {
                std::cout << "        CGI Paths: ";
                for (size_t k = 0; k < cgiPaths.size(); ++k) {
                    std::cout << cgiPaths[k];
                    if (k < cgiPaths.size() - 1) std::cout << ", ";
                }
                std::cout << "\n";
            }
            
            std::cout << "    }\n";
        }
        std::cout << "}\n\n";
    }
    std::cout << "==============================\n";
}