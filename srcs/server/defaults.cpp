#include <sstream>
#include <string>
#include <fcntl.h>
#include "webserv.h"
#include "HeaderGenerator.h"

std::string toString(size_t n) {
    std::ostringstream ss;
    ss << n;
    return ss.str();
}

static const char* generateDefaultPage(int code, size_t* pageSize) {
	if (code == BAD_REQUEST) {
		*pageSize = 228;
		return 
"<!DOCTYPE html>\n\
<html>\n\
<head>\n\
	<meta charset=\"UTF-8\">\n\
	<title>400 Bad request</title>\n\
</head>\n\
<body>\n\
	<h1>400 Bad request</h1>\n\
	<p>The requested resource was not found on this server.</p>\n\
	<hr>\n\
	<p>WebServ 42</p>\n\
</body>\n\
</html>";
	} else if (code == NOT_FOUND) {
		*pageSize = 224;
		return
"<!DOCTYPE html>\n\
<html>\n\
<head>\n\
	<meta charset=\"UTF-8\">\n\
	<title>404 Not Found</title>\n\
</head>\n\
<body>\n\
	<h1>404 Not Found</h1>\n\
	<p>The requested resource was not found on this server.</p>\n\
	<hr>\n\
	<p>WebServ 42</p>\n\
</body>\n\
</html>";
	} else if (code == FORBIDDEN) {
		*pageSize = 223;
		return
"<!DOCTYPE html>\n\
<html>\n\
<head>\n\
	<meta charset=\"UTF-8\">\n\
	<title>403 Forbidden</title>\n\
</head>\n\
<body>\n\
	<h1>403 Forbidden</h1>\n\
	<p>You don't have permission to access on this server.</p>\n\
	<hr>\n\
	<p>WebServ 42</p>\n\
</body>\n\
</html>";
	} else if (code == METHOD_NOT_ALLOWED) {
		*pageSize = 240;
		return
"<!DOCTYPE html>\n\
<html>\n\
<head>\n\
	<meta charset=\"UTF-8\">\n\
	<title>405 Method not allowed</title>\n\
</head>\n\
<body>\n\
	<h1>405 Method not allowed</h1>\n\
	<p>The server does allow you to use this method.</p>\n\
	<hr>\n\
	<p>WebServ 42</p>\n\
</body>\n\
</html>";
	} else if (code == SERVER_ERROR) {
		*pageSize = 297;
		return
"<!DOCTYPE html>\n\
<html>\n\
<head>\n\
	<meta charset=\"UTF-8\">\n\
	<title>500 Internal Server Error</title>\n\
</head>\n\
<body>\n\
	<h1>500 Internal Server Error</h1>\n\
	<p>The server encountered an internal error or misconfiguration and was unable to complete your request.</p>\n\
	<hr>\n\
	<p>WebServ 42</p>\n\
</body>\n\
</html>";
	} else if (code == SERVICE_UNAVAILABLE) {
		*pageSize = 307;
		return 
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"    <meta charset=\"UTF-8\">\n"
"    <title>503 Service Unavailable</title>\n"
"</head>\n"
"<body>\n"
"    <h1>503 Service Unavailable</h1>\n"
"    <p>The server is currently unable to handle the request due to temporary overloading or maintenance.</p>\n"
"    <hr>\n"
"    <p>WebServ 42</p>\n"
"</body>\n"
"</html>";
	} 
	log(ERROR, "Unkown error code");
	return "<h1> SAY MY NAME </h1>";
}

void Server::sendError(int code, Connection& conn) {
    Response& res = conn.res;

	conn.sendBuffer.clear();

    res.status = code;
    res.contentType = "text/html";

    std::string path;
    std::map<int, std::string>::const_iterator it = config_.errorPages.find(code);

    if (it != config_.errorPages.end()) {
        path = it->second;
	}

    if (!path.empty()) {
		if (path[0] != '/') {
			path = config_.root + '/' + path;
		}
		
		if (prepareFileResponse(conn, path)) {
			return modifyToWrite(conn.fd);
		}	

		log(WARNING, "Error page " + path + " not found");
    }

    const char* page;
    size_t pageSize;

    page = generateDefaultPage(code, &pageSize);	
    res.contentLength = toString(pageSize);
    std::string header = generateHeader(res);

    conn.sendBuffer.append(header);
    conn.sendBuffer.append(page, pageSize);

    modifyToWrite(conn.fd);
}
