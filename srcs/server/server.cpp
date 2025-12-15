#include "webserv.h"
#include <unistd.h>  
#include <sys/socket.h>
#include <iostream>
#include <exception>
#include <cstring>

void	log(int	type, const std::string& msg) {
	if (type == INFO) {
		std::cout << "[INFO] <time> " << msg << std::endl;
	} else if (type == ERROR) {
		std::cerr << "[ERROR] <time> " << msg << std::endl;
	}
}

Server::Server() {
	log(INFO, "Starting server with default configuration");
	server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd_ == -1) {
		log(ERROR, "Cannot create socket");
		throw SocketError();
	}
	int	opt = 1;
	setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	addr_ = sockaddr_in();
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = INADDR_ANY;
	addr_.sin_port = htons(config_.port);

	if (bind(server_fd_, (sockaddr*)&addr_, sizeof(addr_)) < 0) {
		log(ERROR, "Bind failed");
		throw SocketError();
	}
	if (listen(server_fd_, 10) < 0) {
		log(ERROR, "Listen failed");
		throw SocketError();
	}
	log(INFO, "Server listening port 8080");
}

Server::Server(std::string& filename) {
	log(INFO, "Starting server");
	log(INFO, "Parsing configuration from " + filename);
	// parse(config_);
	server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd_ == -1) {
		log(ERROR, "Cannot create socket");
		throw SocketError();
	}
	int	opt = 1;
	setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	addr_ = sockaddr_in();
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = INADDR_ANY;
	addr_.sin_port = htons(config_.port);

	if (bind(server_fd_, (sockaddr*)&addr_, sizeof(addr_)) < 0) {
		log(ERROR, "Bind failed");
		throw SocketError();
	}
	if (listen(server_fd_, 10) < 0) {
		log(ERROR, "Listen failed");
		throw SocketError();
	}
	log(INFO, "Server listening port 8080");	
}

Server::~Server() {
	close(server_fd_);
}

const char*	Server::SocketError::what() const throw() {
	return "";
}
