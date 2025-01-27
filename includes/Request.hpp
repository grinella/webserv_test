#pragma once
#include <string>
#include <map>
#include "ServerConfig.hpp"

class Request{
private:
   enum State {
       READ_METHOD,
       READ_HEADERS,
       READ_BODY,
       COMPLETE,
       ERROR
   };

   Request* request;
   State state;
   std::string method;
   std::string uri;
   std::string httpVersion;
   std::map<std::string, std::string> headers;
   std::string body;
   size_t contentLength;
   bool chunked;
   ServerConfig* matchedServer;

   std::string resolvedPath;
   LocationConfig* matchedLocation;

   int deleteStatus; // per lo stato del DELETE, se è andato a buon fine o meno
   int postStatus;

   void parseStartLine(const std::string& line);
   void parseHeader(const std::string& line);
   bool parseChunk();
   void generateDirectoryListing(const std::string& path);

public:
   Request();
   bool parse(const std::string& data);
   void setMatchedServer(ServerConfig* server);

   // Getters
   const std::string& getMethod() const;
   const std::string& getUri() const;
   const std::string& getBody() const;
   bool isComplete() const;
   ServerConfig* getMatchedServer() const;

   void matchLocation(const std::vector<LocationConfig>& locations);
   const std::string& getResolvedPath() const;
   LocationConfig* getMatchedLocation() const;
   bool isMethodAllowed() const;
   void handlePost(const std::string& data);
   void handleDelete();
   int getDeleteStatus() const { return deleteStatus; }
   int getPostStatus() const { return postStatus; }
};