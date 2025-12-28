#include "webserv.h"

void	Server::handleRequest(HttpRequest&	request) {
	// 1) Parse the resquest
	// 2) Get the file name or director  request->http->path
	//    If path ends with / then add default file index.html name to the path
	// 3) Read the file request->http->path
	std::string	path = config_.root + "/index.html";
	std::string	page;

	log(INFO, "<method> <path> <http_version> " + request.get());
	if (!fileExists(path)) {
		return sendError(NOT_FOUND, request);
	}
	if (!canReadFile(path)) {
		return sendError(FORBIDDEN, request);
	}
	page = readFile(path);
	request.sendAll(page);
}

void	Server::acceptConnection() {
	pid_t		pid;
	int			request_fd;
	sockaddr_in	request_addr;
	socklen_t	request_len = sizeof(request_addr);

	request_fd = accept(server_fd_, (sockaddr *)&request_addr, &request_len);
	if (request_fd < 0) {
		log(ERROR, "accept() failed");
		return;
	}
	HttpRequest request(request_fd);
	pid = fork();
	if (pid == 0) {
		handleRequest(request);
		_exit(0);
	} else if (pid < 0) {
		log(ERROR, "Failed to create new process");
		return sendError(SERVER_ERROR, request);
	}
}
