#ifndef HEADERGENERATOR_H
#define HEADERGENERATOR_H

#include <string>
#include <cstring>
#include "RequestParser.h"
#include "defines.h"

struct Response {
    std::string version;
	int status;
    std::string contentType;
    std::string contentLength;
    std::string connectionType;
    SetCookie*  cookie;

    Response()
        : version("HTTP/1.1"),
          status(OK),
          contentType("text/html"),
          contentLength(""),
          connectionType("close"),
          cookie(NULL)
    {}
};

char*	generateHeader(const struct Response& res);

#endif
