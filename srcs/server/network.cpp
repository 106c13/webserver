#include <string.h>
#include <fcntl.h>
#include "webserv.h"


void Server::handleRequest(HttpRequest& request) {
	// 1) Parse the resquest
	// 2) Get the file name or directory from request->http->path
	//    If path ends with / then add default file name like index.html to the path
	// 3) Read the file request->http->path
	std::string path;
	LocationConfig location;
	int fd;
	int status;
	
	parser_.parse(request.get());
	request.setRequest(parser_.getRequest());

	log(request);
	path = request.getPath();
	location = resolve_location(request.getURI(), path);
	status = resolve_path(path, location);
	std::cout << path << std::endl;

	if (status == 1)
		return sendError(NOT_FOUND, request);
	else if (status == 2)
		return sendError(FORBIDDEN, request);


	// 1) When no CGI, just simple read file and send it
	fd = open(path.c_str(), O_RDONLY);
	if (fd < 0) {
		return sendError(SERVER_ERROR, request);
	}
	request.sendAll(path, fd);
	close(fd);

	// response = readFile(path);
	// 2) If CGI is on, then prepare the enviroment
	// fd = runCGI("./php-cgi", request);
	// if (fd < 0)
	// 	return sendError(SERVER_ERROR, request);
	// request.sendAll(fd);
}

void Server::acceptConnection() {
	int			request_fd;
	sockaddr_in	request_addr;
	socklen_t	request_len = sizeof(request_addr);

	request_fd = accept(server_fd_, (sockaddr *)&request_addr, &request_len);
	if (request_fd < 0) {
		log(ERROR, "accept() failed");
		return;
	}
	HttpRequest request(request_fd);
	handleRequest(request);
}
