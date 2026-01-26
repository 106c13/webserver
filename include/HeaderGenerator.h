#ifndef HEADERGENERATOR_H
#define HEADERGENERATOR_H

#include <string>
#include <cstring>
#include "RequestParser.h"
#include "defines.h"

struct Response {
    std::string path;
    std::string version;
	int status;
    std::string contentType;
    std::string contentLength;
    std::string connectionType;
    std::string location;
    SetCookie*  cookie;

    Response()
        : path(""),
          version("HTTP/1.1"),
          status(OK),
          contentType(""),
          contentLength(""),
          connectionType("close"),
          location(""),
          cookie(NULL)
    {}
};

char*	generateHeader(const struct Response& res);

#endif
