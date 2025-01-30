#pragma once
#include <string>
#include <map>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "Request.hpp"

class Response {
private:

   struct EntryCompare {
       bool operator()(const std::pair<std::string, struct stat>& a,
                      const std::pair<std::string, struct stat>& b) const {
           return a.first < b.first;
       }
   };
   
   Request* request;
   int statusCode;
   std::string statusMessage;
   std::map<std::string, std::string> headers;
   std::string body;
   size_t bytesSent;
   std::string response;
   static std::map<std::string, std::string> mimeTypes;

   void buildResponse();
   void setContentType(const std::string& path);
   bool serveStaticFile(const std::string& path);
   void serveErrorPage(int code);
   void serveCGI(Request* req);
   void handleRedirect();
   bool isCGIRequest(const std::string& path);
   char* myStrdup(const char* str);
   void generateDirectoryListing(const std::string& path);
   std::string getCGIInterpreter(const std::string& extension);
   void setupCGIEnv(std::map<std::string, std::string>& env, const std::string& scriptPath);
   std::string executeCGI(const std::string& interpreter, const std::string& scriptPath,
                        const std::map<std::string, std::string>& env);

   std::string parseMultipartData(const std::string& boundary);
   void initMimeTypes();
   std::string intToString(int number);

public:
    explicit Response(Request* request);
    bool send(int fd);
    void setStatus(int code, const std::string& message);
    void setHeader(const std::string& key, const std::string& value);
    void setBody(const std::string& content);
};