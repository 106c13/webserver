#include <sstream>
#include "webserv.hpp"

int	Request::sendAll(const std::string&	response) {
	size_t		total = 0;
	size_t		len = response.length();
	size_t		byteLeft = len;
	ssize_t		n;
	const char*	buf = response.c_str();

	//========== TEST ================
	std::stringstream	ss;
	ss << "HTTP/1.1 200 Ok\r\n";
	ss << "Content-Type: text/html\r\n";
	ss << "Content-Length: " << response.size() << "\n";
	ss << "Connection: close\r\n\n";
	send(request_fd_, ss.str().c_str(), 83, 0);
	//========== TEST ================

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

const std::string&	Request::get() {
	return content_;
}
