#include <sstream>
#include <cstring>
#include <fcntl.h>
#include "webserv.h"
#include "HeaderGenerator.h"

static const char* generateDefaultPage(int code, size_t* pageSize) {
	if (code == NOT_FOUND) {
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
	}
	log(ERROR, "Unkown error code");
	return "";
}

void Server::sendError(int code, HttpRequest& request) const {
	const char* page;
	const char* header;
	size_t		pageSize;
	std::string path;
	std::map<int, std::string>::const_iterator it = config_.errorPages.find(code);

	request.setStatus(code);
	if (it != config_.errorPages.end())
		path = it->second;

	if (!path.empty()) {
		if (request.sendFile(path) > 0)
			return;
	}

	page = generateDefaultPage(code, &pageSize);
	request.setContentLength(pageSize);
	header = generateHeader(request.getResponse());
	request.sendAll(header, std::strlen(header));
	request.sendAll(page, pageSize);
	delete[] header;
}
