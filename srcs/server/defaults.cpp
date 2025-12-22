#include "webserv.hpp"

void	Server::pageNotFound(Request& request) const {
	const char*	page = "<!DOCTYPE html>\n\
<html>\n\
<head>\n\
	<meta charset=\"UTF-8\">\n\
	<title>404 Not Found</title>\n\
</head>\n\
<body>\n\
	<h1>404 Not Found</h1>\n\
	<p>The requested resource was not found on this server.</p>\n\
	<hr>\n\
	<p>WebServer</p>\n\
</body>\n\
</html>";
	request.sendAll(page);
}
