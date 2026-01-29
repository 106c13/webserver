#include <sstream>
#include <fcntl.h>
#include <poll.h>
#include "webserv.h"

int HttpRequest::sendChunked(const int fd) {
	ssize_t bytesRead;
	char buf[1024];
	struct pollfd pfd;
	pfd.fd = request_fd_;
	pfd.events = POLLOUT;

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

int HttpRequest::sendFile(const std::string& path) {
    char    buf[1024];
    ssize_t bytes_read;
    int     fd;

    if (!fileExists(path) || !canReadFile(path)) {
        log(WARNING, "Error page \"" + path + "\" not found");
        return -1;
    }
    fd = open(path.c_str(), O_RDONLY);

    /* ---------- send header ---------- */
    setContentLength(getFileSize(path));
    res_.path = path;
    char* header = generateHeader(res_);
    if (!sendAll(header, std::strlen(header))) {
        delete[] header;
        close(fd);
        return -1;
    }
    delete[] header;

    /* ---------- send file body ---------- */
    struct pollfd pfd;
    pfd.fd = request_fd_;
    pfd.events = POLLOUT;

    while (true) {
        bytes_read = read(fd, buf, sizeof(buf));
        if (bytes_read < 0)
            return close(fd), -1;

        if (bytes_read == 0)
            break;

        size_t total_sent = 0;
        while (total_sent < (size_t)bytes_read) {
            pfd.revents = 0;

            if (poll(&pfd, 1, -1) <= 0)
                return close(fd), -1;

            if (!(pfd.revents & POLLOUT))
                return close(fd), -1;

            ssize_t sent = send(
                request_fd_,
                buf + total_sent,
                bytes_read - total_sent,
                0
            );
            if (sent <= 0)
                return close(fd), -1;

            total_sent += sent;
        }
    }

    close(fd);
    return 1;
}


int HttpRequest::sendAll(const std::string& response) {
    return sendAll(response.c_str(), response.size());
}

int HttpRequest::sendAll(const char* buf, size_t len) {
    size_t total = 0;
    ssize_t n;

    struct pollfd pfd;
    pfd.fd = request_fd_;
    pfd.events = POLLOUT;

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
