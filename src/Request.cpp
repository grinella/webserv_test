#include "../includes/Request.hpp"
#include <sstream>
#include <algorithm> // per std::find
#include <iostream>
#include <sys/stat.h>
#include <cstdlib>
#include <cstdio>
#include <sys/types.h> // Per creare directory
#include <fstream>


Request::Request() : state(READ_METHOD), contentLength(0), chunked(false), matchedServer(NULL) {}

std::string urlDecode(const std::string &encoded) {
    std::ostringstream decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            // Converte il valore in esadecimale in un carattere ASCII
            std::istringstream hex(encoded.substr(i + 1, 2));
            int charCode;
            hex >> std::hex >> charCode;
            decoded << static_cast<char>(charCode);
            i += 2;
        } else if (encoded[i] == '+') {
            // '+' è codificato come spazio
            decoded << ' ';
        } else {
            decoded << encoded[i];
        }
    }
    return decoded.str();
}

bool Request::parse(const std::string& data) {
    std::istringstream iss(data);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.empty() || line == "\r") {
            if (state == READ_HEADERS) {
                state = READ_BODY;
                if (!chunked && contentLength == 0) {
                    state = COMPLETE;
                    std::cout << "getMethod() = " << getMethod() << std::endl;
                    if(getMethod() == "DELETE")
                        handleDelete();
                    std::cout << "ciao1" << std::endl;
                    return true;
                }
                if (contentLength != 0) {
                    std::cout << "in POST getMethod() = " << getMethod() << std::endl;
                    if(getMethod() == "POST")
                        handlePost(data);
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
                    std::cout << "ciao" << std::endl;
                    return true;
                }
                break;
            default:
                break;
        }
    }
    return false;
}

void Request::handleDelete() {
    struct stat pathStat;
    deleteStatus = 200; // Default a success

    // Non modificare l'URI qui
    std::string fullPath = "www" + uri;
    fullPath = urlDecode(fullPath);
    std::cerr << "Full path: " << fullPath << std::endl;

    if (stat(fullPath.c_str(), &pathStat) != 0) {
        perror("stat failed");
        std::cerr << "File or directory does not exist: " << resolvedPath << std::endl;
        deleteStatus = 404;
        return;
    }

    if (S_ISDIR(pathStat.st_mode)) {
        std::cerr << "Cannot delete a directory via DELETE request: " << resolvedPath << std::endl;
        deleteStatus = 403;
        return;
    }

    if (std::remove(fullPath.c_str()) != 0) {
        perror("Error deleting file");
        deleteStatus = 500;
        return;
    }

    std::cout << "File deleted successfully: " << fullPath << std::endl;
}

void Request::handlePost(const std::string& data) {
    std::cout << "ciao3" << std::endl;
    std::cout << "Richiesta:\n" << data << std::endl;
    // Trova l'inizio del corpo della richiesta
    size_t bodyPos = data.find("\r\n\r\n");
    if (bodyPos == std::string::npos) {
        perror("Error: Body not found");
        postStatus = 400;
        return;
    }

    // Estrarre il corpo della richiesta
    std::string body = data.substr(bodyPos + 4);

    std::cout << "body:\n" << body << std::endl;

    // Recupera il boundary dal campo Content-Type
    std::string boundaryMarker = "boundary=";
    size_t boundaryPos = data.find(boundaryMarker);
    if (boundaryPos == std::string::npos) {
        perror("Error: Boundary not found");
        postStatus = 400;
        return;
    }

    size_t boundaryStart = boundaryPos + boundaryMarker.size();
    size_t boundaryEnd = data.find("\r\n", boundaryStart);
    if (boundaryEnd == std::string::npos) {
         perror("Error: Boundary end not found");
        postStatus = 400;
        return;
    }

    // Costruisce il boundary completo
    std::string boundary = "--" + data.substr(boundaryStart, boundaryEnd - boundaryStart);

    // Trova l'inizio della prima parte multipart
    size_t start = body.find(boundary);
    if (start == std::string::npos) {
         perror("Error: Body not found");
        postStatus = 400;
        return;
    }
    start += boundary.size();

    // Trova l'header Content-Disposition per il file
    size_t dispositionPos = body.find("Content-Disposition", start);
    if (dispositionPos == std::string::npos) {
         perror("Error: Content disposition not found");
        postStatus = 400;
        return;
    }

    // Trova il nome del file
    size_t filenamePos = body.find("filename=\"", dispositionPos);
    if (filenamePos == std::string::npos) {
         perror("Error: Filename not found");
        postStatus = 400;
        return;
    }
    filenamePos += 10; // Lunghezza di "filename=\""
    size_t filenameEnd = body.find("\"", filenamePos);
    if (filenameEnd == std::string::npos) {
         perror("Error: Filename end not found");
        postStatus = 400;
        return;
    }
    std::string filename = body.substr(filenamePos, filenameEnd - filenamePos);

    // Trova il contenuto del file
    start = body.find("\r\n\r\n", filenameEnd);
    if (start == std::string::npos) {
        perror("Error: File content not found");
        postStatus = 400;
        return;
    }
    start += 4;

    std::string closingBoundary = boundary + "--";
    size_t end = body.find(closingBoundary, start);
    if (end == std::string::npos) {
        perror("Error: File content end not found");
        postStatus = 400;
        return;
    }
    end -= 2; // Rimuovi "\r\n" prima del boundary

    std::string fileContent = body.substr(start, end - start);

    // Controlla e crea la directory di upload
    std::string uploadDir = "./www/uploads/";
    struct stat info;
    if (stat(uploadDir.c_str(), &info) != 0) { // La directory non esiste
        if (mkdir(uploadDir.c_str(), 0777) != 0) {
            perror("Error: Directory creation failed");
            postStatus = 500;
            return;
        }
    }

    std::string filePath = uploadDir + filename;
    std::ofstream outFile(filePath.c_str(), std::ios::binary);
    if (!outFile.is_open()) {
        perror("Error: file writing failed");
        postStatus = 500;
        return;
    }
    outFile.write(fileContent.c_str(), fileContent.size());
    outFile.close();

    // Risposta di successo
    postStatus = 200;
}

void Request::parseStartLine(const std::string& line) {
    std::istringstream iss(line);
    std::cout << "line=" << line << std::endl;
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
    size_t i = 0;
    while ( i < locations.size()) {
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
                    resolvedPath += normalizedUri.substr(locPath.length() + 1);
                }
            } else {
                resolvedPath = "www" + normalizedUri;
            }
        }
        ++i;
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

const std::string& Request::getMethod() const{
    return this->method;
}

const std::string& Request::getBody() const{
    return this->body;
}

// andra' aggiunto handlePost()