#include "HeaderGenerator.h"
#include "webserv.h"
#include "defines.h"

static std::string getStatus(int code) {
    if (code == OK) {
        return "200 OK";
    } else if (code == REDIRECT) {
        return "301 Moved Permanently";
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

static std::string resolveContentType(const std::string& ext) {
    if (ext == "css")
        return "text/css";
    if (ext == "js")
        return "text/javascript";
    if (ext == "png")
        return "image/png";
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "gif")
        return "image/gif";
    if (ext == "html")
        return "text/html";
    return "application/octet-stream";
}

std::string generateHeader(const struct Response& res) {
    std::string header;
    std::string contentType;
    std::string ext;

    header += res.version;
    header += " ";
    header += getStatus(res.status);
    header += "\r\n";

    if (!res.contentType.empty()) {
        contentType = res.contentType;
    } else {
        size_t dot = res.path.rfind('.');
        if (dot != std::string::npos && dot + 1 < res.path.size())
            ext = res.path.substr(dot + 1);
        contentType = resolveContentType(ext);
    }

    if (!res.location.empty())
        header += "Location: " + res.location + "\r\n";

    header += "Content-Type: " + contentType + "\r\n";
    header += "Content-Length: " + res.contentLength + "\r\n";

    if (!res.contentDisposition.empty())
        header += "Content-Disposition: " + res.contentDisposition + "\r\n";

    if (res.cookie) {
        header += "Set-Cookie: ";
        header += res.cookie->value;
        header += "\r\n";
    }

    header += "Connection: " + res.connectionType + "\r\n";
    header += "\r\n";
	
	return header;
}
