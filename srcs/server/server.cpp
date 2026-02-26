#include <unistd.h>  
#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <exception>
#include <cstring>
#include <sstream>
#include "webserv.h"
#include "ConfigParser.h"

void Server::modifyToWrite(int fd) {
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
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
    std::cout << "Client disconnected..." << std::endl;
}

static bool requestComplete(const Buffer& buf, size_t& endPos) {
    const char* data = buf.data();
    size_t len = buf.size();

    for (size_t i = 0; i + 3 < len; ++i) {
        if (data[i] == '\r' &&
            data[i+1] == '\n' &&
            data[i+2] == '\r' &&
            data[i+3] == '\n')
        {
            endPos = i + 4;
            return true;
        }
    }
    return false;
}

void Server::handleClient(epoll_event& event) {
    int fd = event.data.fd;
	Connection& conn = connections_[fd];

	if (event.events & EPOLLIN) {
        handleRead(conn);
	} else if (event.events & EPOLLOUT) {
        return handleWrite(conn);
	}

    if (conn.state == READING_HEADERS) {
        size_t endPos;

        if (!requestComplete(conn.recvBuffer, endPos))
            return;

        std::string raw(conn.recvBuffer.data(), endPos);

        parser_.parse(raw);
        conn.req = parser_.getRequest();

        if (conn.req.method == "GET") {
        	conn.state = PROCESSING;
            conn.recvBuffer.consume(endPos);
            handleRequest(conn);
            return;
        } else if (conn.req.method == "POST") {
            if (conn.req.headers.find("Content-Length") == conn.req.headers.end())
                return sendError(LENGTH_REQUIRED, conn);
            conn.req.body.append(raw);
            conn.recvBuffer.consume(endPos);
            conn.remainingBody = conn.req.contentLenght;

            //if (conn.remainingBody > conn.configMaxBodySize)
            //    return sendError(PAYLOAD_TOO_LARGE, conn);

            conn.state = READING_BODY;
        } else {
            return sendError(METHOD_NOT_ALLOWED, conn);
        }
    }

    if (conn.state == READING_BODY) {
        size_t available = conn.recvBuffer.size();

        if (available == 0)
            return;

        size_t toConsume = std::min(available, conn.remainingBody);

        conn.req.body.append(conn.recvBuffer.data(), toConsume);

        conn.recvBuffer.consume(toConsume);
        conn.remainingBody -= toConsume;

        if (conn.remainingBody == 0) {
            conn.state = PROCESSING;
            parser_.parse(conn.req.body);
            conn.req = parser_.getRequest();
            handleRequest(conn);
        }
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
        conn.sendingFile = false;
        conn.remainingBody = 0;
        conn.state = READING_HEADERS;

        std::cout << "New connection..." << std::endl;
    }
}

void Server::initSocket() {
	std::ostringstream oss;

	log(INFO, "Starting server");
	serverFd_ = socket(AF_INET, SOCK_STREAM, 0);

	if (serverFd_ < 0) {
		throw std::runtime_error("socket() failed");
	}

	int opt = 1;
	if (setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		throw std::runtime_error("setsockopt() failed");
	}

	addr_ = sockaddr_in();
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = INADDR_ANY;
	addr_.sin_port = htons(config_.port);

	if (bind(serverFd_, (sockaddr*)&addr_, sizeof(addr_)) < 0) {
		oss << "bind() failed on port " << config_.port;
		throw std::runtime_error(oss.str());
	}

	if (listen(serverFd_, 10) < 0) {
		oss << "listen() failed on port " << config_.port;
		throw std::runtime_error(oss.str());
	}
	
	fcntl(serverFd_, F_SETFL, O_NONBLOCK);

	oss << "Server started on port ";
	oss << config_.port;
	log(INFO, oss.str()); 
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

Server::Server(const ServerConfig& config) {
	epollFd_ = epoll_create(1024);
	config_ = config;
	
	initSocket();
	
	fcntl(serverFd_, F_SETFL, O_NONBLOCK);

    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = serverFd_;

    epoll_ctl(epollFd_, EPOLL_CTL_ADD, serverFd_, &ev);
}

Server::~Server() {
	if (serverFd_ >= 0) {
		close(serverFd_);
	}
}
