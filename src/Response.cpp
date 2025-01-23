#include "../includes/Response.hpp"
#include <sstream>
#include <unistd.h>

Response::Response(Request* request) : statusCode(200), statusMessage("OK"), bytesSent(0) {
    (void)request; // per evitare warning unused parameter
    headers["Content-Type"] = "text/html";
    headers["Server"] = "Webserv/1.0";
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
        
    bytesSent += sent;
    return bytesSent >= response.length();
}