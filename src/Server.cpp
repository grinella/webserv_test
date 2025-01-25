#include "../includes/Server.hpp"
#include <iostream>

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

// aggiunto con questo ultimo push 18:12
void Request::setMatchedServer(ServerConfig* server) {
    if (!server) {
        throw std::runtime_error("Attempting to set null server configuration");
    }
    matchedServer = server;
}

void Server::handleRequest(int clientFd) {
    // Prendiamo il client_max_body_size dal config
    size_t maxBodySize = configs.front().getClientMaxBodySize();
    // Allochiamo un buffer della dimensione appropriata
    char* buffer = new char[maxBodySize + 1];  // +1 per il terminatore null
    std::string requestData;
    
    try {
        while (true) {
            ssize_t bytes = read(clientFd, buffer, maxBodySize);
            if (bytes <= 0) {
                if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                    break;
                }
                delete[] buffer;
                removeClient(clientFd);
                return;
            }
            
            // Verifica che non stiamo superando il limite massimo
            if (requestData.length() + bytes > maxBodySize) {
                delete[] buffer;
                // Rispondi con un errore 413 (Payload Too Large)
                Request* request = requests[clientFd];
                request->setMatchedServer(&configs.front());
                responses[clientFd] = new Response(request);
                responses[clientFd]->setStatus(413, "Payload Too Large");
                responses[clientFd]->setBody("Request entity too large");
                
                epoll_event ev;
                ev.events = EPOLLOUT | EPOLLET;
                ev.data.fd = clientFd;
                epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
                return;
            }
            
            buffer[bytes] = '\0';
            requestData += std::string(buffer, bytes);
            
            Request* request = requests[clientFd];
            request->setMatchedServer(&configs.front());
            
            if (request->parse(requestData)) {
                request->matchLocation(configs.front().getLocations());
                responses[clientFd] = new Response(request);
                
                epoll_event ev;
                ev.events = EPOLLOUT | EPOLLET;
                ev.data.fd = clientFd;
                epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev);
                break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling request: " << e.what() << std::endl;
    }
    
    delete[] buffer;  // Libera la memoria del buffer
}
// fino a qui aggiunto e modificato con questo ultimo push 18:12

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