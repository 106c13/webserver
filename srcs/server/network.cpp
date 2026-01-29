#include <fcntl.h>
#include <string>
#include "webserv.h"
#include "ConfigParser.h"

void Server::sendRedirect(HttpRequest& request, const LocationConfig& location) {
	char* header;

	request.setStatus(location.redirectCode);
	request.setLocation(location.redirectUrl);
	header = generateHeader(request.getResponse());
	request.sendAll(header);
	delete[] header;
}

int checkRequest(const Request& request, const LocationConfig& location) {
	if (location.methods.empty()) {
		if (request.method == "GET" ||
			request.method == "POST" ||
			request.method == "DELETE")
			return 1;
        return 0;
	}

    for (std::vector<std::string>::const_iterator it = location.methods.begin();
         it != location.methods.end();
         ++it)
    {
        if (*it == request.method)
            return 1;
    }
    return 0;
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
	if (!checkRequest(request.getRequest(), location))
		return sendError(BAD_REQUEST, request);
	if (location.redirectCode != 0) {
		return sendRedirect(request, location);
	}
	status = resolvePath(path, location);

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
