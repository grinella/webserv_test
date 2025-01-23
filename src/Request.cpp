#include "../includes/Request.hpp"
#include <sstream>
#include <algorithm> // per std::find
#include <iostream>

Request::Request() : state(READ_METHOD), contentLength(0), chunked(false), matchedServer(NULL) {}

bool Request::parse(const std::string& data) {
    std::istringstream iss(data);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.empty() || line == "\r") {
            if (state == READ_HEADERS) {
                state = READ_BODY;
                if (!chunked && contentLength == 0) {
                    state = COMPLETE;
                    return true;
                }
            }
            continue;
        }

        switch (state) {
            case READ_METHOD:
                parseStartLine(line);
                state = READ_HEADERS;
                break;
            case READ_HEADERS:
                parseHeader(line);
                break;
            case READ_BODY:
                body += line + "\n";
                if (!chunked && body.length() >= contentLength) {
                    state = COMPLETE;
                    return true;
                }
                break;
            default:
                break;
        }
    }
    return false;
}

void Request::parseStartLine(const std::string& line) {
    std::istringstream iss(line);
    iss >> method >> uri >> httpVersion;
    if (method.empty() || uri.empty() || httpVersion.empty())
        state = ERROR;
}

void Request::parseHeader(const std::string& line) {
    size_t pos = line.find(':');
    if (pos != std::string::npos) {
        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);
        headers[key] = value;

        if (key == "Content-Length")
            contentLength = std::atoi(value.c_str());
        else if (key == "Transfer-Encoding" && value.find("chunked") != std::string::npos)
            chunked = true;
    }
}

void Request::matchLocation(const std::vector<LocationConfig>& locations) {
    size_t longestMatch = 0;
    
    // Normalizza URI rimuovendo trailing slash
    std::string normalizedUri = uri;
    if (normalizedUri.length() > 1 && normalizedUri[normalizedUri.length()-1] == '/') {
        normalizedUri = normalizedUri.substr(0, normalizedUri.length()-1);
    }
    
    for (size_t i = 0; i < locations.size(); ++i) {
        const LocationConfig& loc = locations[i];
        std::string locPath = loc.getPath();
        
        if (normalizedUri.substr(0, locPath.length()) == locPath && locPath.length() > longestMatch) {
            longestMatch = locPath.length();
            matchedLocation = const_cast<LocationConfig*>(&loc);
            
            if (normalizedUri == "/") {
                resolvedPath = "www/index.html";
            } else if (!matchedLocation->getRoot().empty()) {
                resolvedPath = matchedLocation->getRoot();
                if (normalizedUri != locPath) {
                    resolvedPath += normalizedUri.substr(locPath.length());
                }
            } else {
                resolvedPath = "www" + normalizedUri;
            }
        }
    }
    std::cout << "URI: " << uri << "\nResolved path: " << resolvedPath << std::endl;
}

bool Request::isMethodAllowed() const {
    if (!matchedLocation) return false;
    
    const std::vector<std::string>& allowedMethods = matchedLocation->getAllowedMethods();
    return std::find(allowedMethods.begin(), allowedMethods.end(), method) != allowedMethods.end();
}

const std::string& Request::getResolvedPath() const { return resolvedPath; }
LocationConfig* Request::getMatchedLocation() const { return matchedLocation; }