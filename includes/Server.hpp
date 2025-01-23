#pragma once
#include <vector>
#include <map>
#include <sys/epoll.h>
#include "../includes/ServerConfig.hpp"
#include "../includes/Socket.hpp"
#include "../includes/Request.hpp"
#include "../includes/Response.hpp"

#define MAX_EVENTS 64

class Server {
private:
   std::vector<ServerConfig> configs;
   int epollFd;
   std::map<int, Socket*> sockets;
   std::map<int, Request*> requests;
   std::map<int, Response*> responses;
   
   void setupSockets();
   void handleNewConnection(int fd);
   void handleRequest(int clientFd);
   void handleResponse(int clientFd);
   void removeClient(int clientFd);

public:
   Server(const std::vector<ServerConfig>& configs);
   ~Server();
   void run();
};