#include <unistd.h>  
#include <sys/socket.h>
#include <iostream>
#include <exception>
#include <cstring>
#include <sstream>
#include "webserv.h"
#include "ConfigParser.h"



void Server::initSocket() {
	std::ostringstream oss;

	server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd_ < 0)
		throw std::runtime_error("socket() failed");

	int opt = 1;
	if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("setsockopt() failed");

	// value-initialize (NO memset)
	addr_ = sockaddr_in();
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = INADDR_ANY;
	addr_.sin_port = htons(config_.port);

	if (bind(server_fd_, (sockaddr*)&addr_, sizeof(addr_)) < 0) {
		oss << "bind() failed on port " << config_.port;
		throw std::runtime_error(oss.str());
	}

	if (listen(server_fd_, 10) < 0) {
		oss << "listen() failed on port " << config_.port;
		throw std::runtime_error(oss.str());
	}
}

Server::Server() {
	initSocket();
}

Server::Server(const ServerConfig& config) {
	config_ = config;
	initSocket();
}

Server::~Server() {
	if (server_fd_ >= 0)
		close(server_fd_);
}
