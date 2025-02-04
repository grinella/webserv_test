#include "../includes/Response.hpp"
#include "../includes/Utils.hpp"
#include <sstream>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <dirent.h>
#include <cstdlib>     // Per exit
#include <cstring>     // Per strdup
#include <sys/types.h> // Per pid_t
#include <sys/wait.h>  // Per waitpid
#include <algorithm>

std::map<std::string, std::string> Response::mimeTypes;

void Response::initMimeTypes() {
    mimeTypes.insert(std::make_pair(".html", "text/html"));
    mimeTypes.insert(std::make_pair(".css", "text/css"));
    mimeTypes.insert(std::make_pair(".js", "application/javascript"));
    mimeTypes.insert(std::make_pair(".jpg", "image/jpeg"));
    mimeTypes.insert(std::make_pair(".png", "image/png"));
    mimeTypes.insert(std::make_pair(".ico", "image/x-icon"));
    mimeTypes.insert(std::make_pair(".php", "application/x-httpd-php"));
    mimeTypes.insert(std::make_pair(".py", "text/x-python"));
    mimeTypes.insert(std::make_pair(".sh", "application/x-sh"));
}

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

void Response::handleRedirect() {
    const LocationConfig* loc = request->getMatchedLocation();
    if (!loc) {
        std::cout << "No matched location for redirect" << std::endl;
        return;
    }

    std::cout << "Processing redirect - From: " << request->getUri() 
              << " To: " << loc->getRedirect() 
              << " (Code: " << loc->getRedirectCode() << ")" << std::endl;

    std::cout << "Checking redirect for location: " << loc->getPath() << std::endl;
    if (!loc->hasRedirect()) {
        std::cout << "No redirect found for this location" << std::endl;
        return;
    }

    std::cout << "Processing redirect - Code: " << loc->getRedirectCode() 
              << " Target: " << loc->getRedirect() << std::endl;

    // Imposta lo status code e il messaggio appropriati
    statusCode = loc->getRedirectCode();
    if (statusCode == 301) {
        statusMessage = "Moved Permanently";
    } else {
        statusMessage = "Found";
    }

    // Imposta l'header Location per la redirezione
    headers["Location"] = loc->getRedirect();

    // Crea un semplice body HTML
    std::stringstream body_ss;
    body_ss << "<html><head><title>" << statusCode << " " << statusMessage << "</title></head>";
    body_ss << "<body><h1>" << statusCode << " " << statusMessage << "</h1>";
    body_ss << "<p>The document has been moved to <a href=\"" << loc->getRedirect() << "\">";
    body_ss << loc->getRedirect() << "</a></p></body></html>";
    
    body = body_ss.str();
    headers["Content-Type"] = "text/html";
}

Response::Response(Request* req) : statusCode(200), statusMessage("OK"), bytesSent(0) {
    initMimeTypes();
    request = req;
    
    // Controlla prima se c'è una redirezione
    const LocationConfig* loc = req->getMatchedLocation();
    if (loc && loc->hasRedirect()) {
        std::cout << "Found redirect in location " << loc->getPath() << std::endl;
        handleRedirect();
        return;
    }

    if (!req->getMatchedLocation()) {
        serveErrorPage(404);
        return;
    }

    if (!req->isMethodAllowed()) {
        serveErrorPage(405);
        return;
    }

    // Check for redirection first
    if (req->getMatchedLocation()->hasRedirect()) {
        handleRedirect();
        return;
    }

    if (req->getMethod() == "DELETE") {
        // Usa lo status del DELETE
        int deleteStatus = req->getDeleteStatus();
        if (deleteStatus != 200) {
            serveErrorPage(deleteStatus);
        } else {
            setStatus(200, "OK");
            setBody("File deleted successfully");
        }
        return;
    }
    if (req->getMethod() == "POST") {
        int postStatus = req->getPostStatus();
        if (postStatus != 200) {
            serveErrorPage(postStatus);
        } else {
            setStatus(200, "OK");
            setBody("File uploaded successfully");
        }
        return;
    }

   std::string path = req->getResolvedPath();
   if (isCGIRequest(path)) {
       serveCGI(req);
   } else {
       if (!serveStaticFile(path)) {
           serveErrorPage(404);
       }
   }
}

void Response::buildResponse() {
   std::ostringstream oss;
   oss << "HTTP/1.1 " << statusCode << " " << statusMessage << "\r\n";
   
   for (std::map<std::string, std::string>::const_iterator it = headers.begin(); 
        it != headers.end(); ++it) {
       oss << it->first << ": " << it->second << "\r\n";
   }
   
   oss << "Content-Length: " << body.length() << "\r\n";
   oss << "\r\n";
   oss << body;
   
   response = oss.str();
}

bool Response::send(int fd) {
    if (response.empty())
        buildResponse();
    
    size_t remaining = response.length() - bytesSent;
    if (remaining == 0)
        return true;
        
    ssize_t sent = write(fd, response.c_str() + bytesSent, remaining);
    if (sent <= 0)
        return false;
        
    bytesSent += sent;
    return bytesSent >= response.length();
}

void Response::setContentType(const std::string& path) {
   std::string decodedPath = urlDecode(path);
   size_t dot = decodedPath.find_last_of(".");
   if (dot != std::string::npos) {
       std::string ext = decodedPath.substr(dot);
       if (ext == ".pdf") {
           headers["Content-Type"] = "application/pdf";
           return;
       }
       std::map<std::string, std::string>::const_iterator it = mimeTypes.find(ext);
       if (it != mimeTypes.end()) {
           headers["Content-Type"] = it->second;
           return;
       }
   }
   headers["Content-Type"] = "application/octet-stream";
}

bool Response::serveStaticFile(const std::string& path) {
    // Decodifica il path e verifica se è una directory
    std::string decodedPath = urlDecode(path);
    struct stat st;
    
    if (stat(decodedPath.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            if (request->getMatchedLocation()->getAutoindex()) {
                generateDirectoryListing(decodedPath);
                return true;
            }
            return false;
        }
    }

    // Apri e leggi il file in modalità binaria
    std::ifstream file(decodedPath.c_str(), std::ios::binary);
    if (!file) {
        return false;
    }

    // Imposta il content type basato sul path decodificato
    setContentType(decodedPath);

    // Leggi il file in un buffer binario
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (file.read(&buffer[0], size)) {
        body = std::string(buffer.begin(), buffer.end());
    } else {
        return false;
    }

    return true;
}

std::string Response::intToString(int number) {
    std::ostringstream ss;
    ss << number;
    return ss.str();
}

void Response::serveErrorPage(int code) {
   statusCode = code;
   switch (code) {
       case 404: statusMessage = "Not Found"; break;
       case 405: statusMessage = "Method Not Allowed"; break;
       case 408: statusMessage = "Timeout"; break;
       case 413: statusMessage = "Payload Too Large"; break;
       case 500: statusMessage = "Internal Server Error"; break;
       default: statusMessage = "Unknown Error";
   }

   std::string errorPath = "www/" + intToString(code) + ".html";
   if (!serveStaticFile(errorPath)) {
       body = "<html><body><h1>" + intToString(code) + " " + statusMessage + "</h1></body></html>";
       headers["Content-Type"] = "text/html";
   }
}

bool Response::isCGIRequest(const std::string& path) {
   size_t dot = path.find_last_of(".");
   if (dot != std::string::npos) {
       std::string ext = path.substr(dot);
       return (ext == ".php" || ext == ".py" || ext == ".sh");
   }
   return false;
}

void Response::generateDirectoryListing(const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        serveErrorPage(404);
        return;
    }

    std::stringstream content;
    // Estrai il path relativo dalla cartella www
    std::string relativePath = path;
    if (relativePath.substr(0, 4) == "www/") {
        relativePath = relativePath.substr(4);
    }
    if (!relativePath.empty() && relativePath[relativePath.length()-1] != '/') {
        relativePath += '/';
    }

    // Crea l'header della pagina con stili CSS inline
    content << "<html><head><title>Index of /" << relativePath << "</title>\n";
    content << "<style>\n";
    content << "body { font-family: Arial, sans-serif; margin: 40px; }\n";
    content << "h1 { color: #333; margin-bottom: 20px; }\n";
    content << ".directory-list { width: 100%; border-collapse: collapse; }\n";
    content << ".directory-list th, .directory-list td { padding: 10px; text-align: left; border-bottom: 1px solid #ddd; }\n";
    content << ".directory-list tr:hover { background-color: #f5f5f5; }\n";
    content << "a { color: #0366d6; text-decoration: none; }\n";
    content << "a:hover { text-decoration: underline; }\n";
    content << ".size { color: #666; }\n";
    content << ".date { color: #666; }\n";
    content << "</style></head>";
    content << "<body>\n";
    content << "<h1>Index of /" << relativePath << "</h1>\n";
    content << "<table class=\"directory-list\">\n";
    content << "<tr><th>Name</th><th>Size</th><th>Last Modified</th></tr>\n";

    struct dirent* entry;
    struct stat fileStat;
    std::string fullPath;
    
    // Raccogli tutte le entries in un vector per poterle ordinare
    std::vector<std::pair<std::string, struct stat> > entries;
    
    while ((entry = readdir(dir))) {
        std::string name = entry->d_name;
        if (name != "." && name != "..") {
            fullPath = path + "/" + name;
            if (stat(fullPath.c_str(), &fileStat) == 0) {
                entries.push_back(std::make_pair(name, fileStat));
            }
        }
    }

    // Ordina le entries per nome
    std::sort(entries.begin(), entries.end(), EntryCompare());

    // Aggiungi un link per tornare alla directory superiore se non siamo nella root
    if (relativePath != "") {
        content << "<tr><td><a href=\"/";
        size_t lastSlash = relativePath.substr(0, relativePath.length()-1).find_last_of('/');
        if (lastSlash != std::string::npos) {
            content << relativePath.substr(0, lastSlash + 1);
        }
        content << "\">..</a></td><td>-</td><td>-</td></tr>\n";
    }

    // Genera le righe della tabella per ogni entry
    for (std::vector<std::pair<std::string, struct stat> >::const_iterator it = entries.begin();
         it != entries.end(); ++it) {
        const std::string& name = it->first;
        const struct stat& st = it->second;

        content << "<tr><td><a href=\"/" << relativePath << name << "\">" 
                << name << "</a></td>";

        // Dimensione del file
        if (S_ISDIR(st.st_mode)) {
            content << "<td>-</td>";
        } else {
            if (st.st_size < 1024) {
                content << "<td class=\"size\">" << st.st_size << " B</td>";
            } else if (st.st_size < 1024 * 1024) {
                content << "<td class=\"size\">" << st.st_size / 1024 << " KB</td>";
            } else {
                content << "<td class=\"size\">" << st.st_size / (1024 * 1024) << " MB</td>";
            }
        }

        // Data di ultima modifica
        char timeStr[80];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
        content << "<td class=\"date\">" << timeStr << "</td></tr>\n";
    }

    content << "</table></body></html>";
    closedir(dir);

    body = content.str();
    headers["Content-Type"] = "text/html";
}

std::string Response::getCGIInterpreter(const std::string& extension) {
    const LocationConfig* loc = request->getMatchedLocation();
    if (!loc) return "";

    const std::vector<std::string>& extensions = loc->getCgiExtensions();
    const std::vector<std::string>& paths = loc->getCgiPaths();
    
    for (size_t i = 0; i < extensions.size(); ++i) {
        if (extension == extensions[i] && i < paths.size()) {
            return paths[i];
        }
    }
    return "";
}

const std::string& Request::getUri() const {
    return this->uri;
}

void Response::setupCGIEnv(std::map<std::string, std::string>& env, const std::string& scriptPath) {
    std::string path = request->getResolvedPath();
    std::string documentRoot = "./www"; // Configurabile dal config file

    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["REQUEST_METHOD"] = request->getMethod();
    env["SCRIPT_FILENAME"] = scriptPath;
    env["SCRIPT_NAME"] = request->getUri();
    env["QUERY_STRING"] = request->getQueryString();
    env["REMOTE_ADDR"] = "127.0.0.1";
    env["REQUEST_URI"] = request->getUri();
    env["SERVER_SOFTWARE"] = "webserv/1.0";
    env["PATH_INFO"] = "";
    env["PATH_TRANSLATED"] = scriptPath;

    if (request->getMethod() == "POST") {
        env["CONTENT_TYPE"] = request->getHeader("Content-Type");
        env["CONTENT_LENGTH"] = Utils::U_intToString(request->getBody().length());
        env["HTTP_CONTENT_TYPE"] = request->getHeader("Content-Type");
        env["HTTP_CONTENT_LENGTH"] = Utils::U_intToString(request->getBody().length());
    }

    // Aggiungi headers HTTP come variabili d'ambiente
    const std::map<std::string, std::string>& headers = request->getAllHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string name = "HTTP_" + it->first;
        std::replace(name.begin(), name.end(), '-', '_');
        env[name] = it->second;
    }
}

char* Response::myStrdup(const char* str) {
    size_t len = strlen(str) + 1;
    char* new_str = new char[len];
    std::strcpy(new_str, str);
    return new_str;
}

std::string Response::executeCGI(const std::string& interpreter, const std::string& scriptPath,
                                const std::map<std::string, std::string>& env) {
    int input_pipe[2];  // Per inviare dati al CGI
    int output_pipe[2]; // Per ricevere dati dal CGI
    
    if (pipe(input_pipe) < 0 || pipe(output_pipe) < 0) {
        throw std::runtime_error("Failed to create pipes");
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(input_pipe[0]); close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        throw std::runtime_error("Fork failed");
    }

    if (pid == 0) { // Processo figlio
        // Reindirizza stdin e stdout ai pipe
        close(input_pipe[1]);
        dup2(input_pipe[0], STDIN_FILENO);
        close(input_pipe[0]);

        close(output_pipe[0]);
        dup2(output_pipe[1], STDOUT_FILENO);
        close(output_pipe[1]);

        // Prepara l'ambiente
        std::vector<std::string> envStrs;
        for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it) {
            envStrs.push_back(it->first + "=" + it->second);
        }

        char** envp = new char*[envStrs.size() + 1];
        for (size_t i = 0; i < envStrs.size(); ++i) {
            envp[i] = strdup(envStrs[i].c_str());
        }
        envp[envStrs.size()] = NULL;

        // Esegui il CGI
        char* const args[] = {
            const_cast<char*>(interpreter.c_str()),
            const_cast<char*>(scriptPath.c_str()),
            NULL
        };

        execve(interpreter.c_str(), args, envp);
        exit(1); // Se execve fallisce
    }

    // Processo padre: monitora il figlio e gestisce il timeout
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[1]);

    int status;
    int timeout = 10;  // Timeout in secondi
    bool process_terminated = false;

    for (int i = 0; i < timeout; ++i) {
        sleep(1);
        pid_t result = waitpid(pid, &status, WNOHANG);

        if (result == pid) { // Il processo è terminato
            process_terminated = true;
            break;
        }
    }

    if (!process_terminated) {
        std::cerr << "⚠️ Timeout raggiunto! Terminazione del processo CGI...\n";
        kill(pid, SIGKILL);  // Termina il processo CGI
        waitpid(pid, &status, 0);  // Evita processi zombie

        serveErrorPage(408);
        return body;
    }

    // Lettura dell'output del CGI
    char buffer[1024];
    std::string output;
    ssize_t bytesRead;
    while ((bytesRead = read(output_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    close(output_pipe[0]);
    return output;
}

void Response::serveCGI(Request* req) {
    try {
        std::string path = req->getResolvedPath();
        std::string extension = path.substr(path.find_last_of("."));
        std::string interpreter = getCGIInterpreter(extension);
        
        std::map<std::string, std::string> env;
        setupCGIEnv(env, path);

        if (req->getMethod() == "POST") {
            env["CONTENT_LENGTH"] = Utils::U_intToString(req->getBody().length());
            env["CONTENT_TYPE"] = req->getHeader("Content-Type");
            
            if (!req->getBody().empty()) {
                setPOSTData(req->getBody());  // Nuovo metodo da implementare
            }
        }

        std::string output = executeCGI(interpreter, path, env);
        if (output.empty()) {
            throw std::runtime_error("Empty CGI output");
        }
        processOutput(output);
        if (!headers.count("Content-Type")) {
            headers["Content-Type"] = "text/html";
        }

    } catch (const std::exception& e) {
        std::cerr << "CGI Error: " << e.what() << std::endl;
        serveErrorPage(500);
    }
}

void Response::setPOSTData(const std::string& data) {
    int pipes[2];
    if (pipe(pipes) == -1) {
        throw std::runtime_error("Failed to create pipe for POST data");
    }
    write(pipes[1], data.c_str(), data.length());
    close(pipes[1]);
    dup2(pipes[0], STDIN_FILENO);
    close(pipes[0]);
}

void Response::processOutput(const std::string& output) {
    std::string::size_type headerEnd = output.find("\r\n\r\n");
    if (headerEnd == std::string::npos) headerEnd = output.find("\n\n");
    
    if (headerEnd != std::string::npos) {
        parseHeaders(output.substr(0, headerEnd));
        body = output.substr(headerEnd + (output[headerEnd + 1] == '\n' ? 2 : 4));
    } else {
        headers["Content-Type"] = "text/html";
        body = output;
    }
    
    statusCode = 200;
    statusMessage = "OK";
}

void Response::parseHeaders(const std::string& headerSection) {
    std::istringstream headerStream(headerSection);
    std::string line;
    
    while (std::getline(headerStream, line)) {
        if (line.empty() || line == "\r") continue;
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of("\r") + 1);
            headers[key] = value;
        }
    }
}

void Response::setStatus(int code, const std::string& message) {
   statusCode = code;
   statusMessage = message;
}

void Response::setHeader(const std::string& key, const std::string& value) {
   headers[key] = value;
}

void Response::setBody(const std::string& content) {
   body = content;
}
std::string Response::parseMultipartData(const std::string& boundary) {
   std::string content = request->getBody();
   size_t start = content.find(boundary);
   if (start == std::string::npos) return "";

   size_t filenamePos = content.find("filename=\"", start);
   if (filenamePos == std::string::npos) return "";
   
   filenamePos += 10;
   size_t filenameEnd = content.find("\"", filenamePos);
   if (filenameEnd == std::string::npos) return "";

   return content.substr(filenamePos, filenameEnd - filenamePos);
}
