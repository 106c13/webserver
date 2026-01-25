#include "HeaderGenerator.h"
#include "webserv.h"
#include "defines.h"

static std::string getStatus(int code) {
    if (code == OK) {
        return "200 OK";
    } else if (code == BAD_REQUEST) {
        return "400 Bad Request";
    } else if (code == FORBIDDEN) {
        return "403 Forbidden";
    } else if (code == NOT_FOUND) {
        return "404 Not Found";
    } else if (code == SERVER_ERROR) {
        return "500 Internal Server Error";
    } else {
        return "500 Internal Server Error";
    }
}

char* generateHeader(const struct Response& res)
{
    std::string header;

    header += res.version;
    header += " ";
    header += getStatus(res.status);
    header += "\r\n";

    if (!res.contentType.empty()) {
        header += "Content-Type: ";
        header += res.contentType;
        header += "\r\n";
    }

    if (!res.contentLength.empty()) {
        header += "Content-Length: ";
        header += res.contentLength;
        header += "\r\n";
    }

    if (!res.connectionType.empty()) {
        header += "Connection: ";
        header += res.connectionType;
        header += "\r\n";
    }

    if (res.cookie) {
        header += "Set-Cookie: ";
        header += res.cookie->value;
        header += "\r\n";
    }

    header += "\r\n";

    char* result = new char[header.size() + 1];
    std::memcpy(result, header.c_str(), header.size());
    result[header.size()] = '\0';

    return result;
}
