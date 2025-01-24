#include "../includes/Response.hpp"
#include <sstream>
#include <unistd.h>
#include <fstream>  // per std::ifstream
#include <iostream>  // per std::cout

const std::map<std::string, std::string> Response::mimeTypes = {
   {".html", "text/html"},
   {".css", "text/css"},
   {".js", "application/javascript"},
   {".jpg", "image/jpeg"},
   {".png", "image/png"},
   {".ico", "image/x-icon"}
};

Response::Response(Request* req) : statusCode(200), statusMessage("OK"), bytesSent(0) {
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
    
    std::cout << response << std::endl;
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

void Response::serveErrorPage(int code) {
   statusCode = code;
   switch (code) {
       case 404: statusMessage = "Not Found"; break;
       case 405: statusMessage = "Method Not Allowed"; break;
       case 500: statusMessage = "Internal Server Error"; break;
       default: statusMessage = "Unknown Error";
   }

   std::string errorPath = "www/" + std::to_string(code) + ".html";
   if (!serveStaticFile(errorPath)) {
       body = "<html><body><h1>" + std::to_string(code) + " " + statusMessage + "</h1></body></html>";
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

void Response::serveCGI(Request* req) {
   // TODO: Implement CGI handling
   (void)req;  // per evitare il warning unused parameter
   serveErrorPage(500);
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