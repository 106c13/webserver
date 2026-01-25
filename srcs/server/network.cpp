#include <fcntl.h>
#include <string>
#include "webserv.h"


void Server::sendRedirect(HttpRequest& request, std::string url) {
	std::string redirectHeader;

	redirectHeader = "HTTP/1.1 301\n";
	redirectHeader += "Location: " + url + "\n";
	request.sendAll(redirectHeader);
}

/*
 * Parse the resquest
 * Resolve the path
 * If path ends with / then add default file name like index.html to the path
 * If autoindex is on and no satisfying page is found, generate autoindex
 * If page found, then check does there a cgi for that page
 * If cgi found, run cgi and send the output from cgi
 * If cgi not found, just read the file
*/
void Server::handleRequest(HttpRequest& request) {
	std::string path;
	std::string cgiPath;
	LocationConfig location;
	int fd;
	int status;
	
	parser_.parse(request.getContent());
	request.setRequest(parser_.getRequest());

	log(request);
	path = request.getPath();
	location = resolveLocation(path);
	if (location.redirectCode != 0 && 1 == 2) {
		return sendRedirect(request, location.redirectUrl);
	}
	status = resolvePath(path, location);
	std::cout << path << " " << status << std::endl;

	if (status == 1) {
		// If autoindex is on, generate page
		if (location.autoindex)
			return generateAutoindex(request, location);
		log(INFO, "404 " + path);
		return sendError(NOT_FOUND, request);
	} else if (status == 2) {
		return sendError(NOT_FOUND, request);
	} else if (status == 3) {
		log(INFO, "403 " + path);
		return sendError(FORBIDDEN, request);
	}

	cgiPath = findCGI(request.getPath(), location.cgi);
	request.setPath(path);
	
	if (!cgiPath.empty()) {
		return sendError(SERVER_ERROR, request);
		fd = runCGI(path.c_str(), cgiPath.c_str(), request);
		std::cout << "fd=" << fd << std::endl;
		if (fd < 0)
			return sendError(SERVER_ERROR, request);
		//request.sendChunked(fd);
	} else {
		if (request.sendFile(path) < 0)
			return sendError(SERVER_ERROR, request);
	}
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
