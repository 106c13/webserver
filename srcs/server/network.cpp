#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include "webserv.hpp"
#include <unistd.h>

void	Server::acceptConnection() {
	int			client_fd;
	sockaddr_in	client_addr;
	socklen_t	client_len = sizeof(client_addr);

	client_fd = accept(server_fd_, (sockaddr *)&client_addr, &client_len);
	if (client_fd < 0) {
		log(ERROR, "accept() failed");
		return;
	}
	std::string	msg = "42 WebServ";
	send(client_fd, msg.c_str(), msg.length(), 0);

	close(client_fd);
}
