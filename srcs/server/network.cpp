#include "webserv.hpp"

void	Server::acceptConnection() {
	int			request_fd;
	sockaddr_in	request_addr;
	socklen_t	request_len = sizeof(request_addr);

	request_fd = accept(server_fd_, (sockaddr *)&request_addr, &request_len);
	if (request_fd < 0) {
		log(ERROR, "accept() failed");
		return;
	}
	Request request(request_fd);
	std::cout << request.get() << std::endl;
	request.sendAll("42 webserv");
}
