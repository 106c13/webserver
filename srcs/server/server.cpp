#include <unistd.h>  
#include <fcntl.h>
#include <sys/socket.h>
#include <iostream>
#include <exception>
#include <cstring>
#include <sstream>
#include "webserv.h"
#include "ConfigParser.h"



void Server::initSocket() {
	std::ostringstream oss;

	log(INFO, "Starting server");
	serverFd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd_ < 0)
		throw std::runtime_error("socket() failed");

	int opt = 1;
	if (setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("setsockopt() failed");

	// value-initialize (NO memset)
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
	if (serverFd_ >= 0)
		close(serverFd_);
}
