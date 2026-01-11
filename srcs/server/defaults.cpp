#include <sstream>
#include "webserv.h"

void Server::sendError(int code, HttpRequest& request) const {
	std::string			page;

	if (code == NOT_FOUND) {
		page =
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
		page = "<!DOCTYPE html>\n\
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
		page = "<!DOCTYPE html>\n\
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
	request.sendAll(page.c_str());
}
