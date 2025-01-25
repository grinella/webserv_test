#include "../includes/Response.hpp"
#include <sstream>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <cstdlib>     // Per exit
#include <cstring>     // Per strdup
#include <sys/types.h> // Per pid_t
#include <sys/wait.h>  // Per waitpid

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

Response::Response(Request* req) : statusCode(200), statusMessage("OK"), bytesSent(0) {
    initMimeTypes();
    request = req;
    
    if (!req->getMatchedLocation()) {
        serveErrorPage(404);
        return;
    }

    if (!req->isMethodAllowed()) {
        serveErrorPage(405);
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
       
   ssize_t sent = write(fd, response.c_str() + bytesSent, response.length() - bytesSent);
   if (sent == -1)
       return false;
    
    // std::cout << response << std::endl;
   bytesSent += sent;
   // cout true or false
   
   return bytesSent >= response.length();
}

void Response::setContentType(const std::string& path) {
   size_t dot = path.find_last_of(".");
   if (dot != std::string::npos) {
       std::string ext = path.substr(dot);
       std::map<std::string, std::string>::const_iterator it = mimeTypes.find(ext);
       if (it != mimeTypes.end()) {
           headers["Content-Type"] = it->second;
           return;
       }
   }
   headers["Content-Type"] = "text/plain";
}

bool Response::serveStaticFile(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            if (request->getMatchedLocation()->getAutoindex()) {
                generateDirectoryListing(path);
                return true;
            }
            return false;
        }
    }

    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file) return false;

    setContentType(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    body = buffer.str();
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
    content << "<html><head><title>Index of " << path << "</title></head><body>\n";
    content << "<h1>Index of " << path << "</h1><hr><pre>\n";

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (std::string(entry->d_name) != "." && std::string(entry->d_name) != "..") {
            content << "<a href=\"" << entry->d_name << "\">" << entry->d_name << "</a>\n";
        }
    }
    content << "</pre><hr></body></html>";
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
    env["GATEWAY_INTERFACE"] = "CGI/1.1";
    env["SERVER_PROTOCOL"] = "HTTP/1.1";
    env["REQUEST_METHOD"] = request->getMethod();
    env["SCRIPT_FILENAME"] = scriptPath;
    env["SCRIPT_NAME"] = request->getUri();
    env["PATH_INFO"] = "";
    env["PATH_TRANSLATED"] = scriptPath;
    env["QUERY_STRING"] = "";  // TODO: Implementare il parsing della query string
    env["CONTENT_TYPE"] = "text/html";
    env["CONTENT_LENGTH"] = "0";
    
    // Se ci sono headers della richiesta specifici per CGI, aggiungili qui
    // HTTP_* headers
}

char* Response::myStrdup(const char* str) {
    size_t len = strlen(str) + 1;
    char* new_str = new char[len];
    std::strcpy(new_str, str);
    return new_str;
}

std::string Response::executeCGI(const std::string& interpreter, const std::string& scriptPath,
                                const std::map<std::string, std::string>& env) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        throw std::runtime_error("Failed to create pipe");
    }

    pid_t pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        throw std::runtime_error("Fork failed");
    }

    if (pid == 0) { // Child process
        close(pipefd[0]); // Close read end
        dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
        close(pipefd[1]);

        // Prepare environment variables for execve
        std::vector<std::string> envStrs;
        for (std::map<std::string, std::string>::const_iterator it = env.begin(); it != env.end(); ++it) {
            envStrs.push_back(it->first + "=" + it->second);
        }

        char** envp = new char*[envStrs.size() + 1];
        for (size_t i = 0; i < envStrs.size(); ++i) {
            envp[i] = myStrdup(envStrs[i].c_str());
        }
        envp[envStrs.size()] = NULL;

        // Prepare arguments for execve
        char* const args[] = {
            const_cast<char*>(interpreter.c_str()),
            const_cast<char*>(scriptPath.c_str()),
            NULL
        };

        execve(interpreter.c_str(), args, envp);
        
        // If execve fails
        for (size_t i = 0; envp[i] != NULL; ++i) {
            delete[] envp[i];
        }
        delete[] envp;
        
        exit(1);
    }

    // Parent process
    close(pipefd[1]); // Close write end

    // Read output from pipe
    std::string output;
    char buffer[4096];
    ssize_t bytes_read;
    
    while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        output += buffer;
    }
    
    close(pipefd[0]);

    // Wait for child process
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        throw std::runtime_error("CGI script execution failed");
    }

    return output;
}

void Response::serveCGI(Request* req) {
    try {
        std::string path = req->getResolvedPath();
        size_t dot = path.find_last_of(".");
        if (dot == std::string::npos) {
            throw std::runtime_error("No file extension found");
        }

        std::string extension = path.substr(dot);
        std::string interpreter = getCGIInterpreter(extension);
        if (interpreter.empty()) {
            throw std::runtime_error("No interpreter found for extension: " + extension);
        }

        std::map<std::string, std::string> env;
        setupCGIEnv(env, path);

        std::string output = executeCGI(interpreter, path, env);
        
        // Parse CGI output (separate headers from body)
        size_t headerEnd = output.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            headerEnd = output.find("\n\n");
        }
        
        if (headerEnd != std::string::npos) {
            // Process CGI headers
            std::string headers = output.substr(0, headerEnd);
            std::istringstream headerStream(headers);
            std::string line;
            
            while (std::getline(headerStream, line) && !line.empty()) {
                size_t colonPos = line.find(':');
                if (colonPos != std::string::npos) {
                    std::string key = line.substr(0, colonPos);
                    std::string value = line.substr(colonPos + 1);
                    // Trim whitespace
                    value.erase(0, value.find_first_not_of(" \t"));
                    this->headers[key] = value;
                }
            }
            
            body = output.substr(headerEnd + 4); // Skip \r\n\r\n
        } else {
            // No headers found, treat everything as body
            body = output;
            headers["Content-Type"] = "text/html";
        }
        
        statusCode = 200;
        statusMessage = "OK";
    } catch (const std::exception& e) {
        std::cerr << "CGI Error: " << e.what() << std::endl;
        serveErrorPage(500);
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