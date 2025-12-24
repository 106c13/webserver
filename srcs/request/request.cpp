#include "webserv.hpp"

Request::Request(int fd) : request_fd_(fd) {
	char	buff[1024] = {0};

	read(request_fd_, buff, 1024);

	content_ = buff;
}

Request::~Request() {
	close(request_fd_);
}
