#include "webserv.h"

HttpRequest::HttpRequest(int fd) : request_fd_(fd) {
	char	buff[1024] = {0};

	read(request_fd_, buff, 1024);

	content_ = buff;
}

HttpRequest::~HttpRequest() {
	close(request_fd_);
}


void HttpRequest::setRequest(const Request& request) {
	body_ = request;
}

void HttpRequest::setFile(const std::string& path) {
	body_.path = path;
}

const std::string& HttpRequest::get() {
	return content_;
}

const std::string& HttpRequest::getPath() const {
	return body_.path;
}

const std::string& HttpRequest::getMethod() const {
	return body_.method;
}
