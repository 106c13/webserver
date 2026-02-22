#include <sys/epoll.h>
#include <cerrno>
#include <fcntl.h>
#include <string>
#include "webserv.h"
#include "ConfigParser.h"


int checkRequest(const Request& request, const LocationConfig& location) {
	if (location.methods.empty()) {
		if (request.method == "GET" ||
			request.method == "POST" ||
            request.method == "DELETE")
        {
			return 1;
        }
        return 0;
	}

    for (std::vector<std::string>::const_iterator it = location.methods.begin();
         it != location.methods.end();
         ++it) {
        if (*it == request.method) {
            return 1;
        }
    }
    return 0;
}

void Server::sendRedirect(Connection& conn, const LocationConfig& location) {
	Response& res = conn.res;
	std::string header;


	res.status = location.redirectCode;
	res.location = location.redirectUrl;
	res.contentLength = "0";

	header = generateHeader(res);
    conn.sendBuffer.append(header);

	std::cout << conn.sendBuffer.data() << std::endl;
    modifyToWrite(conn.fd);
}

/*
 * Parse the resquest
 * Resolve the path
 * If path ends with / then add default file name like index.html to the path
 * If autoindex is on and no satisfying page is found, generate autoindex
 * If page found, then check does there a cgi for that page
 * If cgi found, run cgi and send the output from cgi
 * If cgi not found, just read the file
*/
/*
void Server::handleRequest(Connection& conn) {
	std::string path;
	std::string cgiPath;
	LocationConfig location;
	int fd;
	int status;
	
parser_.parse(conn.recvBuffer.data());

	log(WARNING, "Here");
	path = request.getPath();
	location = resolveLocation(path);
	if (!checkRequest(request.getRequest(), location))
		return sendError(BAD_REQUEST, request);
	if (location.redirectCode != 0) {
		return sendRedirect(request, location);
	}
	status = resolvePath(path, location);

	if (status == 1) {
		// If autoindex is on, generate page
		if (location.autoindex)
			return generateAutoindex(request, location);
		log(INFO, "404 " + path);
		return sendError(NOT_FOUND, request);
	} else if (status == 2) {
		return sendError(NOT_FOUND, request);
	} else if (status == 3) {
		log(INFO, "403 " + path);
		return sendError(FORBIDDEN, request);
	}

	cgiPath = findCGI(request.getPath(), location.cgi);
	request.setPath(path);
	
	if (!cgiPath.empty()) {
		return sendError(SERVER_ERROR, request);
		fd = runCGI(path.c_str(), cgiPath.c_str(), request);
		if (fd < 0)
			return sendError(SERVER_ERROR, request);
		//request.sendChunked(fd);
	} else {
		if (request.sendFile(path) < 0)
			return sendError(SERVER_ERROR, request);
	}
}
*/

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
    res.status = OK;
    res.path = path;
    res.contentLength = toString(size);
    res.connectionType = "close";

    std::string header = generateHeader(res);
    conn.sendBuffer.append(header);

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

void Server::handleRequest(Connection& conn) {
	Request& req = conn.req;

	log(INFO, req.version + " " + req.method + " " + req.uri);

    std::string path = req.path;


	LocationConfig location = resolveLocation(path);

	if (!checkRequest(req, location)) {
		return sendError(BAD_REQUEST, conn);
    } else if (location.redirectCode != 0) {
		return sendRedirect(conn, location);
    }

	int status = resolvePath(path, location);
	std::cout << "Status: " << status << std::endl;
	std::cout << "Path: " << path << std::endl;
	
	if (status == 1) {
		if (location.autoindex) {
			return generateAutoindex(conn, location);
        }
		return sendError(NOT_FOUND, conn);
	} else if (status == 2) {
		return sendError(NOT_FOUND, conn);
	} else if (status == 3) {
		return sendError(FORBIDDEN, conn);
	}
    
    std::string cgiPath = findCGI(path, location.cgi);
	
	if (!cgiPath.empty()) {
        conn.req.path = path;
		int fd = runCGI(path.c_str(), cgiPath.c_str(), conn);
		if (fd < 0) {
			return sendError(SERVER_ERROR, conn);
        }
        return sendCGIOutput(conn, fd);
	} else {
        if (!prepareFileResponse(conn, path))
            return sendError(SERVER_ERROR, conn);
        streamFileChunk(conn);
	}
}


void Server::acceptConnection() {
    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    while (true) {
        int clientFd = accept(serverFd_, (sockaddr*)&addr, &len);
		if (clientFd < 0) {
    		return;
		}

        fcntl(clientFd, F_SETFL, O_NONBLOCK);

        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = clientFd;

        epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev);

		Connection& conn = connections_[clientFd];
		conn.fd = clientFd;
        conn.fileFd = -1;
        conn.sendingFile = false;

        std::cout << "New connection\n";
    }
}


void Server::handleClient(epoll_event& event) {
    int fd = event.data.fd;

	if (event.events & EPOLLIN) {
        handleRead(connections_[fd]);
	} else if (event.events & EPOLLOUT) {
        handleWrite(connections_[fd]);
	}
}

static bool requestComplete(const Buffer& buf, size_t& endPos)
{
    const char* data = buf.data();
    size_t len = buf.size();

    for (size_t i = 0; i + 3 < len; ++i) {
        if (data[i] == '\r' &&
            data[i+1] == '\n' &&
            data[i+2] == '\r' &&
            data[i+3] == '\n')
        {
            endPos = i + 4; // save the end position
            return true;
        }
    }
    return false;
}

void Server::handleRead(Connection& conn) {
    char buf[4096];

    while (true) {
        ssize_t n = recv(conn.fd, buf, sizeof(buf), 0);

        if (n > 0) {
            conn.recvBuffer.append(buf, n);
        } else if (n == 0) {
            closeConnection(conn.fd);
            return;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            closeConnection(conn.fd);
            return;
        }
    }

	size_t endPos;
	while (requestComplete(conn.recvBuffer, endPos)) {
    	std::string raw(conn.recvBuffer.data(), endPos);

		parser_.parse(raw);
		conn.req = parser_.getRequest();

		conn.recvBuffer.consume(endPos);

		handleRequest(conn);
	}

    if (!conn.sendBuffer.empty()) {
        modifyToWrite(conn.fd);
    }
}

void Server::handleWrite(Connection& conn) {
    while (!conn.sendBuffer.empty()) {
        ssize_t n = send(conn.fd,
                         conn.sendBuffer.data(),
                         conn.sendBuffer.size(),
                         0);
        if (n > 0) {
            conn.sendBuffer.consume(n);
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            closeConnection(conn.fd);
            return;
        }
    }
    
    if (conn.sendingFile) {
        streamFileChunk(conn);
    }

    if (conn.sendBuffer.empty()) {
        modifyToRead(conn.fd);
    }
}

void Server::modifyToWrite(int fd) {
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT; // keep reading + allow writing
    ev.data.fd = fd;

    epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

void Server::modifyToRead(int fd) {
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;

    epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

void Server::closeConnection(int fd) {
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
    connections_.erase(fd);
}

void Server::loop() {
    epoll_event events[1024];

    while (true) {
        int evCount = epoll_wait(epollFd_, events, 1024, -1);
        if (evCount < 0)
            continue;

        for (int i = 0; i < evCount; i++) {
            int fd = events[i].data.fd;
            epoll_event& ev = events[i];

            if (fd == serverFd_) {
                acceptConnection();
            } else {
                handleClient(ev);
            }
        }
    }
}
