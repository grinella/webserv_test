#pragma once
#include <string>
#include <map>
#include "ServerConfig.hpp"

class Request{
private:
   std::string requestBuffer;  // Buffer per i dati in arrivo
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

   size_t headerLength;
   bool headerComplete;
   // size_t contentLength;

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
   void handlePost();
   void handleDelete();
   int getDeleteStatus() const { return deleteStatus; }
   int getPostStatus() const { return postStatus; }
   bool isHeaderComplete() const { return headerComplete; }
   size_t getHeaderLength() const { return headerLength; }
   size_t getContentLength() const { return contentLength; }
};