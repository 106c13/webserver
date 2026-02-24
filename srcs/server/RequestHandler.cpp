#include "webserv.h"
#include "ConfigParser.h"

int checkRequest(const Request& request, const LocationConfig& location) {
	if (location.methods.empty()) {
		if (request.method == "GET" ||
			request.method == "POST" ||
            request.method == "DELETE")
        {
			return 1;
        }
        return 0;
	}

    for (std::vector<std::string>::const_iterator it = location.methods.begin();
         it != location.methods.end();
         ++it) {
        if (*it == request.method) {
            return 1;
        }
    }
    return 0;
}

void Server::handleRequest(Connection& conn) {
	Request& req = conn.req;

	log(INFO, req.version + " " + req.method + " " + req.uri);

    std::string path = req.path;


	LocationConfig location = resolveLocation(path);

	if (!checkRequest(req, location)) {
		return sendError(BAD_REQUEST, conn);
    } else if (location.redirectCode != 0) {
		return sendRedirect(conn, location);
    }

	int status = resolvePath(path, location);
	
	if (status == 1) {
		if (location.autoindex) {
			return generateAutoindex(conn, location);
        }
		return sendError(NOT_FOUND, conn);
	} else if (status == 2) {
		return sendError(NOT_FOUND, conn);
	} else if (status == 3) {
		return sendError(FORBIDDEN, conn);
	}
    
    std::string cgiPath = findCGI(path, location.cgi);
	
	if (!cgiPath.empty()) {
        conn.req.path = path;
		int fd = runCGI(path.c_str(), cgiPath.c_str(), conn);
		if (fd < 0) {
			return sendError(SERVER_ERROR, conn);
        }
        return sendCGIOutput(conn, fd);
	} else {
        if (!prepareFileResponse(conn, path))
            return sendError(SERVER_ERROR, conn);
        streamFileChunk(conn);
	}
}

