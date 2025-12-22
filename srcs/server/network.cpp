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
	// 1) Parse the resquest
	// 2) Get the file name or director  request->http->path
	//    If path ends with / then add default file index.html name to the path
	// 3) Read the file request->http->path
	std::string	path = config_.homeDir + "/index.html";

	log(INFO, "<method> <path> <http_version> " + request.get());
	if (!fileExists(path)) {
		return pageNotFound(request);
	}
	request.sendAll("42 webserv");
}
