#include <sstream>
#include "webserv.h"

HttpRequest::HttpRequest(int fd) : request_fd_(fd) {
    char buff[1024] = {0};
    ssize_t bytes_read = read(request_fd_, buff, sizeof(buff) - 1);
    if (bytes_read > 0)
        content_.assign(buff, bytes_read);
}

HttpRequest::~HttpRequest() {
    close(request_fd_);
}

const std::string& HttpRequest::getContent() {
    return content_;
}

// --- Request setters/getters ---

void HttpRequest::setRequest(const Request& request) {
    req_ = request;
}

void HttpRequest::setPath(const std::string& path) {
    req_.path = path;
}

const struct Request& HttpRequest::getRequest() {
    return req_;
}

const std::string& HttpRequest::getPath() const {
    return req_.path;
}

const std::string& HttpRequest::getMethod() const {
    return req_.method;
}

const std::string& HttpRequest::getURI() const {
    return req_.uri;
}

// --- Response setters/getters ---

void HttpRequest::setStatus(int code) {
    res_.status = code;
}

void HttpRequest::setContentType(const std::string& s) {
    res_.contentType = s;
}

void HttpRequest::setLocation(const std::string& s) {
    res_.location = s;
}

void HttpRequest::setContentLength(size_t len) {
    std::stringstream ss;
    ss << len;
    res_.contentLength = ss.str();
}

void HttpRequest::setConnectionType(const std::string& s) {
    res_.connectionType = s;
}

const struct Response& HttpRequest::getResponse() {
    return res_;
}
