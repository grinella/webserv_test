#pragma once
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

class Socket {
private:
    int fd;
    struct sockaddr_in addr;
    bool isNonBlocking;
    Socket(const Socket&);
    Socket& operator=(const Socket&);

public:
    Socket(int port);
    ~Socket();
    int getFd() const;
    void setNonBlocking();
    void bind();
    void listen(int backlog = 10);
    int accept();
};