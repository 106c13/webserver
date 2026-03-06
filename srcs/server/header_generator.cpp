#include "HeaderGenerator.h"
#include "webserv.h"
#include "defines.h"

static std::string getStatus(int code) {
    if (code == OK) {
        return "200 OK";
    } else if (code == NO_CONTENT) {
        return "204 No Content";
    } else if (code == REDIRECT) {
        return "301 Moved Permanently";
    } else if (code == TEMPRORARY_REDIRECT) {
        return "302 Temporary Redirect";
    } else if (code == BAD_REQUEST) {
        return "400 Bad Request";
    } else if (code == FORBIDDEN) {
        return "403 Forbidden";
    } else if (code == NOT_FOUND) {
        return "404 Not Found";
    } else if (code == METHOD_NOT_ALLOWED) {
        return "405 Method Not Allowed";
    } else if (code == LENGTH_REQUIRED) {
        return "411 Length Required";
    } else if (code == PAYLOAD_TOO_LARGE) {
        return "413 Content Too Large";
    } else if (code == SERVICE_UNAVAILABLE) {
        return "503 Service Unavailable";
    }
    return "500 Internal Server Error";
}

static std::string resolveContentType(const std::string& ext) {
    if (ext == "css") {
        return "text/css";
    } else if (ext == "js") {
        return "text/javascript";
    } else if (ext == "png") {
        return "image/png";
    } else if (ext == "jpg" || ext == "jpeg") {
        return "image/jpeg";
    } else if (ext == "gif") {
        return "image/gif";
    } else if (ext == "html") {
        return "text/html";
    }
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

    if (!res.location.empty()) {
        header += "Location: " + res.location + "\r\n";
    }

    header += "Content-Type: " + contentType + "\r\n";
    header += "Content-Length: " + res.contentLength + "\r\n";

    if (!res.contentDisposition.empty()) {
        header += "Content-Disposition: " + res.contentDisposition + "\r\n";
    }

    if (res.cookie) {
        header += "Set-Cookie: ";
        header += res.cookie->value;
        header += "\r\n";
    }

    header += "Connection: " + res.connectionType + "\r\n";
    header += "\r\n";
	
	return header;
}
