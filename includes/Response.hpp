#pragma once
#include <string>
#include <map>
#include <dirent.h>
#include <sys/stat.h>
#include "Request.hpp"

class Response {
private:
   Request* request;  // Add this first
   int statusCode;
   std::string statusMessage;
   std::map<std::string, std::string> headers;
   std::string body;
   size_t bytesSent;
   std::string response;
   static const std::map<std::string, std::string> mimeTypes;

   void buildResponse();
   void setContentType(const std::string& path);
   bool serveStaticFile(const std::string& path);
   void serveErrorPage(int code);
   void serveCGI(Request* req);
   bool isCGIRequest(const std::string& path);
   void generateDirectoryListing(const std::string& path);  // Add this second

   bool handleGet();
   bool handlePost();
   bool handleDelete();
   bool uploadFile(const std::string& fileContent, const std::string& filename);
   std::string parseMultipartData(const std::string& boundary);

public:
   explicit Response(Request* request);
   bool send(int fd);
   void setStatus(int code, const std::string& message);
   void setHeader(const std::string& key, const std::string& value);
   void setBody(const std::string& content);
};