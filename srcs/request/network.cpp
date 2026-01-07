#include <sstream>
#include <poll.h>
#include "webserv.h"

int HttpRequest::sendAll(const int fd) {
	ssize_t bytesRead;
	char buf[1024];
	struct pollfd pfd;
	pfd.fd = request_fd_;
	pfd.events = POLLOUT;

	// ============= TEST ===============
	std::stringstream ss;
	ss << "HTTP/1.1 200 OK\r\n";
	ss << "Content-Type: text/html\r\n";
	ss << "Transfer-Encoding: chunked\r\n";
	ss << "Connection: close\r\n\r\n";

	if (poll(&pfd, 1, -1) <= 0)
		return -1;
	if (send(request_fd_, ss.str().c_str(), ss.str().size(), 0) <= 0)
		return -1;
	// ==================================

	while (true) {
		bytesRead = read(fd, buf, sizeof(buf));
		if (bytesRead < 0)
			return -1;
		if (bytesRead == 0)
			break;

		pfd.revents = 0;
		if (poll(&pfd, 1, -1) <= 0)
			return -1;
		if (!(pfd.revents & POLLOUT))
			return -1;
		std::stringstream chunkSizeStream;
		chunkSizeStream << std::hex << bytesRead << "\r\n";
		std::string chunkSizeStr = chunkSizeStream.str();
		if (send(request_fd_, chunkSizeStr.c_str(), chunkSizeStr.size(), 0) <= 0)
			return -1;
		if (send(request_fd_, buf, bytesRead, 0) <= 0)
			return -1;
		if (send(request_fd_, "\r\n", 2, 0) <= 0)
			return -1;
	}
	const char* endChunk = "0\r\n\r\n";
	if (send(request_fd_, endChunk, 5, 0) <= 0)
		return -1;
	return 1;
}

int HttpRequest::sendAll(const std::string& response) {
	size_t total = 0;
	size_t len = response.size();
	ssize_t n;
	const char* buf = response.c_str();

	struct pollfd pfd;
	pfd.fd = request_fd_;
	pfd.events = POLLOUT;

	//========== TEST ================
	std::stringstream	ss;
	ss << "HTTP/1.1 200 Ok\r\n";
	ss << "Content-Type: text/html\r\n";
	ss << "Content-Length: " << response.size() << "\n";
	ss << "Connection: close\r\n\n";
	if (poll(&pfd, 1, -1) <= 0)
		return 0;
	send(request_fd_, ss.str().c_str(), 83, 0);
	//========== TEST ================

	while (total < len) {
		pfd.revents = 0;

		if (poll(&pfd, 1, -1) <= 0)
			return 0;

		if (pfd.revents & POLLOUT) {
			n = send(request_fd_, buf + total, len - total, 0);
			if (n <= 0)
				return 0;

			total += n;
		}
	}
	return 1;
}

int HttpRequest::sendAll(const std::string& path, int fd) {
    ssize_t totalFileSize = getFileSize(path);
    char buf[1024];
    struct pollfd pfd;
    pfd.fd = request_fd_;
    pfd.events = POLLOUT;

    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "Content-Type: text/html\r\n";
    ss << "Content-Length: " << totalFileSize << "\r\n";
    ss << "Connection: close\r\n\r\n";

    if (poll(&pfd, 1, -1) <= 0)
        return 0;
    if (send(request_fd_, ss.str().c_str(), ss.str().size(), 0) <= 0)
        return 0;

    ssize_t bytesRead;
    while ((bytesRead = read(fd, buf, sizeof(buf))) > 0) {
        ssize_t totalSent = 0;
        while (totalSent < bytesRead) {
            if (poll(&pfd, 1, -1) <= 0)
                return 0;
            if (pfd.revents & POLLOUT) {
                ssize_t sent = send(request_fd_, buf + totalSent, bytesRead - totalSent, 0);
                if (sent <= 0)
                    return 0;
                totalSent += sent;
            }
        }
    }
    return bytesRead == 0 ? 1 : 0; // return 1 if fully sent, 0 otherwise
}

const std::string&	HttpRequest::get() {
	return content_;
}

const std::string&	HttpRequest::getFile() const {
	return file_;
}

void	HttpRequest::setFile(const std::string& file) {
	file_ = file;
}
