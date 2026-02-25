#include "webserv.h"
#include "ConfigParser.h"

int checkRequest(const Request& request, const LocationConfig& location) {
	if (request.method != "GET" && 
		request.method != "POST" &&
        request.method != "DELETE")
    {
		return BAD_REQUEST;
    }

	if (location.methods.empty()) {
        return OK;
	}

	std::cout <<  "Method: " << request.method << std::endl;
    for (std::vector<std::string>::const_iterator it = location.methods.begin();
         it != location.methods.end();
         ++it) {
        if (*it == request.method) {
            return OK;
        }
    }
    return METHOD_NOT_ALLOWED;
}

void Server::handleRequest(Connection& conn) {
    Request& req = conn.req;
    Response& res = conn.res;
    log(INFO, req.version + " " + req.method + " " + req.uri);
    
    std::cout << req.body << std::endl; 


    LocationConfig location = resolveLocation(req.path);

    int status = checkRequest(req, location);
    if (status != OK)
        return sendError(status, conn);

    if (location.redirectCode != 0)
        return sendRedirect(conn, location);

    status = resolvePath(req.path, location);

    if (status == 1 && location.autoindex)
        return generateAutoindex(conn, location);
	else if (status != OK)
        return sendError(status, conn);

    res.path = req.path;
    if (req.method == "POST") {
        res.status = OK;
        res.contentLength = "0";

        for (std::vector<MultipartPart>::iterator it = req.multipartParts.begin();
             it != req.multipartParts.end();
             it++) {
            std::cout << "Name: " << (*it).name << std::endl;
            std::cout << "Data:" << (*it).data << std::endl;
        }
        std::string header = generateHeader(conn.res);
        std::cout << "==================\n";
        std::cout << header;
        conn.sendBuffer.append(header);

        //conn.state = WRITING;
        modifyToWrite(conn.fd);
        return;
    }

    if (!prepareFileResponse(conn, req.path))
        return sendError(SERVER_ERROR, conn);

    streamFileChunk(conn);
	modifyToWrite(conn.fd);
}
