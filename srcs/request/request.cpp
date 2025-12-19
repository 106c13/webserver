#include "webserv.hpp"

Request::Request(int fd) : request_fd_(fd) {
	char	buff[1024] = {0};

	read(request_fd_, buff, 1024);

	content_ = buff;
}

Request::~Request() {
	close(request_fd_);
}

const std::string&	Request::get() {
	return content_;
}

int	Request::sendAll(const std::string&	response) {
	size_t		total = 0;
	size_t		len = response.length();
	size_t		byteLeft = len;
	ssize_t		n;
	const char*	buf = response.c_str();
	
	while (total < len) {
		n = send(request_fd_, buf + total, byteLeft, 0);
		if (n < 0) {
			return -1;
		}
		total += n;
		byteLeft -= n;
	}
	return 1;
}
