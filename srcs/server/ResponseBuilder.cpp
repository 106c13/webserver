#include <sys/epoll.h>
#include <fcntl.h>
#include <string>
#include "webserv.h"
#include "ConfigParser.h"

void Server::sendRedirect(Connection& conn, const LocationConfig& location) {
	Response& res = conn.res;
	std::string header;


	res.status = location.redirectCode;
	res.location = location.redirectUrl;
	res.contentLength = "0";

	header = generateHeader(res);
    conn.sendBuffer.append(header);

    modifyToWrite(conn.fd);
}

bool Server::prepareFileResponse(Connection& conn, const std::string& path) {
    ssize_t size = getFileSize(path);
    if (size < 0) {
        return false;
    }

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return false;
    }

    Response& res = conn.res;
    res.path = path;
    res.contentLength = toString(size);
    res.connectionType = "close";

    std::string header = generateHeader(res);
    conn.sendBuffer.append(header);

	log(INFO, header.substr(0, header.find("\n")));

    conn.fileFd = fd;
    conn.sendingFile = true;

    return true;
}

bool Server::streamFileChunk(Connection& conn) {
    if (conn.fileFd < 0)
        return false;

    char buf[4096];
    ssize_t n = read(conn.fileFd, buf, sizeof(buf));

    if (n > 0) {
        conn.sendBuffer.append(buf, n);
        return true;
    }

    close(conn.fileFd);
    conn.fileFd = -1;
    conn.sendingFile = false;
    return false;
}

