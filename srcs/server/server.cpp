#include "webserv.hpp"
#include <unistd.h>  
#include <sys/socket.h>
#include <iostream>
#include <exception>
#include <cstring>
#include <sstream>

Server::Server() {
	std::ostringstream oss;

	log(INFO, "Starting server with default configuration");
	server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd_ == -1) {
		throw std::runtime_error("Cannot create a socket");
	}
	int	opt = 1;
	setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	addr_ = sockaddr_in();
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = INADDR_ANY;
	addr_.sin_port = htons(config_.port);

	if (bind(server_fd_, (sockaddr*)&addr_, sizeof(addr_)) < 0) {
		oss << "Bind failed on port " << config_.port;
		throw std::runtime_error(oss.str());
	}
	if (listen(server_fd_, 10) < 0) {
		oss << "Listen failed on port " << config_.port;
		throw std::runtime_error(oss.str());

	}
	log(INFO, "Server listening port 8080");
}

Server::Server(std::string& filename) {
	std::ostringstream oss;

	log(INFO, "Starting server");
	log(INFO, "Parsing configuration from " + filename);
	// parse(config_);
	server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd_ == -1) {
		throw std::runtime_error("Cannot create a socket");
	}
	int	opt = 1;
	setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	addr_ = sockaddr_in();
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = INADDR_ANY;
	addr_.sin_port = htons(config_.port);

	if (bind(server_fd_, (sockaddr*)&addr_, sizeof(addr_)) < 0) {
		oss << "Bind failed on port " << config_.port;
		throw std::runtime_error(oss.str());
	}
	if (listen(server_fd_, 10) < 0) {
		oss << "Listen failed on port " << config_.port;
		throw std::runtime_error(oss.str());
	}
	log(INFO, "Server listening port 8080");	
}

Server::~Server() {
	close(server_fd_);
}
