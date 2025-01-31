#include "../includes/Socket.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>

Socket::Socket(int port) : fd(-1), isNonBlocking(false) {
   fd = socket(AF_INET, SOCK_STREAM, 0);
   if (fd == -1)
       throw std::runtime_error("Failed to create socket");

   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = INADDR_ANY;
   addr.sin_port = htons(port);
}

Socket::~Socket() {
   if (fd != -1)
       close(fd);
}

int Socket::getFd() const {
   return fd;
}

void Socket::setNonBlocking() {
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1)
       throw std::runtime_error("Failed to get socket flags");
   
   if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
       throw std::runtime_error("Failed to set socket non-blocking");
   
   isNonBlocking = true;
}

void Socket::bind() {
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
       throw std::runtime_error("Failed to set socket options");

    if (::bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
       throw std::runtime_error("Failed to bind socket");
}

void Socket::listen(int backlog) {
    if (::listen(fd, backlog) == -1)
       throw std::runtime_error("Failed to listen on socket");
}

int Socket::accept() {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    int clientFd = ::accept(fd, (struct sockaddr*)&clientAddr, &addrLen);
    if (clientFd < 0)  // Qualsiasi errore viene gestito come fallimento
        return -1;
        
    return clientFd;
}