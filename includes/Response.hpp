#pragma once
#include <string>
#include <map>
#include "Request.hpp"

class Response {
private:
   int statusCode;
   std::string statusMessage;
   std::map<std::string, std::string> headers;
   std::string body;
   size_t bytesSent;
   std::string response;

   void setContentType(const std::string& path);
   bool serveStaticFile(const std::string& path);
   void serveErrorPage(int code);
   void serveCGI(Request* req);
   bool isCGIRequest(const std::string& path);
   void buildResponse();

   static const std::map<std::string, std::string> mimeTypes;

public:
   Response(Request* request);
   bool send(int fd);
   void setStatus(int code, const std::string& message);
   void setHeader(const std::string& key, const std::string& value);
   void setBody(const std::string& content);
};