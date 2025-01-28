#include "../includes/Request.hpp"
#include <sstream>
#include <algorithm> // per std::find
#include <iostream>
#include <sys/stat.h>
#include <cstdlib>
#include <cstdio>
#include <sys/types.h> // Per creare directory
#include <fstream>


Request::Request() 
    : state(READ_METHOD)
    , contentLength(0)
    , chunked(false)
    , matchedServer(NULL)
    , headerLength(0)
    , headerComplete(false) {}

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
    if (!headerComplete) {
        // Aggiungi i nuovi dati al buffer
        requestBuffer += data;
        
        size_t headerEnd = requestBuffer.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            headerLength = headerEnd + 4;
            headerComplete = true;
            
            // Parse headers
            std::istringstream headerStream(requestBuffer.substr(0, headerEnd));
            std::string line;
            
            // Parse request line
            std::getline(headerStream, line);
            if (!line.empty() && line[line.length()-1] == '\r') {
                line = line.substr(0, line.length()-1);
            }
            parseStartLine(line);
            std::cout << "Method: " << method << ", URI: " << uri << std::endl;
            
            // Parse headers
            while (std::getline(headerStream, line)) {
                if (!line.empty() && line[line.length()-1] == '\r') {
                    line = line.substr(0, line.length()-1);
                }
                if (!line.empty()) {
                    parseHeader(line);
                }
            }
            
            // Se ci sono dati dopo gli headers, sono parte del body
            if (headerEnd + 4 < requestBuffer.length()) {
                body += requestBuffer.substr(headerEnd + 4);
            }
            requestBuffer.clear(); // Pulisci il buffer dopo aver processato gli headers
            
            if (method != "POST") {
                return true;
            }
        }
    } else if (method == "POST") {
        // Aggiungi i nuovi dati direttamente al body
        body += data;
        std::cout << "Added " << data.length() << " bytes to body. Total body size: " << body.length() << std::endl;
        
        // Se abbiamo ricevuto tutti i dati, processa il POST
        if (body.length() >= contentLength) {
            handlePost();
            return true;
        }
    }
    
    return headerComplete && (method != "POST" || body.length() >= contentLength);
}

void Request::handleDelete() {
    struct stat pathStat;
    deleteStatus = 200; // Default a success

    // Decodifica l'URI e costruisci il percorso
    std::string decodedUri = urlDecode(uri);
    std::string fullPath = "www" + decodedUri;
    std::cout << "Attempting to delete file: " << fullPath << std::endl;

    if (stat(fullPath.c_str(), &pathStat) != 0) {
        perror("stat failed");
        std::cerr << "File or directory does not exist: " << fullPath << std::endl;
        deleteStatus = 404;
        return;
    }

    if (S_ISDIR(pathStat.st_mode)) {
        std::cerr << "Cannot delete a directory via DELETE request: " << fullPath << std::endl;
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

void Request::handlePost() {
    std::cout << "Processing POST request" << std::endl;
    
    // Il body dovrebbe già essere stato estratto durante il parsing
    if (body.empty()) {
        std::cerr << "Error: Empty body" << std::endl;
        postStatus = 400;
        return;
    }

    // Recupera il boundary dal Content-Type header
    std::string contentType = headers["Content-Type"];
    size_t boundaryPos = contentType.find("boundary=");
    if (boundaryPos == std::string::npos) {
        std::cerr << "Error: No boundary found in Content-Type" << std::endl;
        postStatus = 400;
        return;
    }

    std::string boundary = "--" + contentType.substr(boundaryPos + 9); // +9 per saltare "boundary="
    std::cout << "Boundary: " << boundary << std::endl;

    // Trova l'inizio dei dati del file
    size_t startPos = body.find(boundary);
    if (startPos == std::string::npos) {
        std::cerr << "Error: Boundary not found in body" << std::endl;
        postStatus = 400;
        return;
    }

    // Trova l'header Content-Disposition
    size_t headerStart = body.find("Content-Disposition:", startPos);
    if (headerStart == std::string::npos) {
        std::cerr << "Error: Content-Disposition not found" << std::endl;
        postStatus = 400;
        return;
    }

    // Estrai il nome del file
    size_t filenamePos = body.find("filename=\"", headerStart);
    if (filenamePos == std::string::npos) {
        std::cerr << "Error: Filename not found" << std::endl;
        postStatus = 400;
        return;
    }
    
    filenamePos += 10; // Lunghezza di 'filename="'
    size_t filenameEnd = body.find("\"", filenamePos);
    if (filenameEnd == std::string::npos) {
        std::cerr << "Error: Invalid filename format" << std::endl;
        postStatus = 400;
        return;
    }

    std::string filename = body.substr(filenamePos, filenameEnd - filenamePos);
    std::cout << "Filename: " << filename << std::endl;

    // Trova l'inizio del contenuto del file (dopo gli headers)
    size_t contentStart = body.find("\r\n\r\n", filenameEnd);
    if (contentStart == std::string::npos) {
        std::cerr << "Error: File content start not found" << std::endl;
        postStatus = 400;
        return;
    }
    contentStart += 4; // Salta \r\n\r\n

    // Trova la fine del contenuto del file
    size_t contentEnd = body.find(boundary, contentStart);
    if (contentEnd == std::string::npos) {
        std::cerr << "Error: File content end not found" << std::endl;
        postStatus = 400;
        return;
    }
    contentEnd -= 4; // Per rimuovere \r\n-- prima del boundary

    // Estrai il contenuto del file
    std::string fileContent = body.substr(contentStart, contentEnd - contentStart);

    // Crea la directory uploads se non esiste
    std::string uploadDir = "./www/uploads/";
    struct stat st;
    if (stat(uploadDir.c_str(), &st) != 0) {
        if (mkdir(uploadDir.c_str(), 0777) != 0) {
            std::cerr << "Error: Failed to create upload directory" << std::endl;
            postStatus = 500;
            return;
        }
    }

    // Scrivi il file
    std::string filePath = uploadDir + filename;
    std::ofstream outFile(filePath.c_str(), std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error: Failed to open file for writing: " << filePath << std::endl;
        postStatus = 500;
        return;
    }

    outFile.write(fileContent.c_str(), fileContent.size());
    outFile.close();

    if (outFile.fail()) {
        std::cerr << "Error: Failed to write file" << std::endl;
        postStatus = 500;
        return;
    }

    std::cout << "File saved successfully: " << filePath << std::endl;
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