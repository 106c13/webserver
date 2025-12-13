#include "webserv.h"
#include <sys/socket.h>
#include <iostream>

Server::Server() {
	log(INFO, "Starting server");
	port_ = 8080;
	server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd_ == -1) {
		log(ERROR, "Cannot create socket");
	}
	int	opt = 1;
	setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	std::memset(&addr_, 0, sizeof(addr));
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = INADDR_ANY;
	addr_.sin_port = htons(8080);

	if (bind(server_fd_, (sockadd*)&addr_, sizeof(addr_)) < 0) {
		log(ERROR, "Bind failed");
	}
	if (listen(server_fd_, 10) < 0) {
		log(ERROR, "Listen failed");
	}
	log(INFO, "Server listening port 8080");
}

Server::~Server() {
	close(server_fd_);
}
