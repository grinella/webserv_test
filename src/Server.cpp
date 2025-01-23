#include "../includes/Server.hpp"

Server::Server(const std::vector<ServerConfig>& configs) : configs(configs) {
   epollFd = epoll_create1(0);
   if (epollFd == -1)
       throw std::runtime_error("Failed to create epoll instance");
   setupSockets();
}

Server::~Server() {
   for (std::map<int, Socket*>::iterator it = sockets.begin(); it != sockets.end(); ++it)
       delete it->second;
   for (std::map<int, Request*>::iterator it = requests.begin(); it != requests.end(); ++it)
       delete it->second;
   for (std::map<int, Response*>::iterator it = responses.begin(); it != responses.end(); ++it)
       delete it->second;
   close(epollFd);
}

void Server::setupSockets() {
   for (std::vector<ServerConfig>::const_iterator it = configs.begin(); it != configs.end(); ++it) {
       Socket* socket = new Socket(it->getPort());
       socket->setNonBlocking();
       socket->bind();
       socket->listen();
       
       epoll_event ev;
       ev.events = EPOLLIN;
       ev.data.fd = socket->getFd();
       
       if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socket->getFd(), &ev) == -1) {
           delete socket;
           throw std::runtime_error("Failed to add socket to epoll");
       }
       
       sockets[socket->getFd()] = socket;
   }
}

void Server::handleNewConnection(int fd) {
    Socket* socket = sockets[fd];
    int clientFd = socket->accept();
    
    if (clientFd == -1)
        return;

    // Imposta client socket non bloccante
    int flags = fcntl(clientFd, F_GETFL, 0);
    fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

    // Aggiungi al epoll
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // Edge Triggered
    ev.data.fd = clientFd;
    
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ev) == -1) {
        close(clientFd);
        return;
    }

    requests[clientFd] = new Request();
}

void Server::handleRequest(int clientFd) {
   char buffer[4096];
   ssize_t bytes = read(clientFd, buffer, sizeof(buffer) - 1);
   
   if (bytes <= 0) {
       removeClient(clientFd);
       return;
   }
   
   buffer[bytes] = '\0';
   Request* req = requests[clientFd];
   
   if (req->parse(std::string(buffer))) {
       req->matchLocation(configs.front().getLocations());  // Per ora usiamo solo il primo server
       responses[clientFd] = new Response(req);
       
       epoll_event ev;
       ev.events = EPOLLOUT | EPOLLET;
       ev.data.fd = clientFd;
       epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
   }
}

void Server::handleResponse(int clientFd) {
   if (responses.find(clientFd) == responses.end()) {
       removeClient(clientFd);
       return;
   }
   
   Response* res = responses[clientFd];
   if (res->send(clientFd)) {
       removeClient(clientFd);
   }
}

void Server::removeClient(int clientFd) {
    epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
    close(clientFd);
    
    delete requests[clientFd];
    delete responses[clientFd];
    requests.erase(clientFd);
    responses.erase(clientFd);
}

void Server::run() {
   epoll_event events[MAX_EVENTS];
   
   while (true) {
       int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);
       if (nfds == -1) {
           if (errno == EINTR)
               continue;
           throw std::runtime_error("epoll_wait failed");
       }
       
       for (int n = 0; n < nfds; ++n) {
           int fd = events[n].data.fd;
           
           if (sockets.find(fd) != sockets.end()) {
               handleNewConnection(fd);
           } else {
               if (events[n].events & EPOLLIN) {
                   handleRequest(fd);
               }
               if (events[n].events & EPOLLOUT) {
                   handleResponse(fd);
               }
           }
       }
   }
}