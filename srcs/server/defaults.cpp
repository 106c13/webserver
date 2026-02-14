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

void Server::sendError(int code, Connection& conn)
{
    Response& res = conn.res;

    res.status = code;
    res.contentType = "text/html";
    res.connectionType = "close";

    const char* page;
    size_t pageSize;

    std::string path;
    std::map<int, std::string>::const_iterator it = config_.errorPages.find(code);

    if (it != config_.errorPages.end())
        path = it->second;

	conn.sendBuffer.clear();

    if (!path.empty()) {
		if (path[0] != '/')
			path = config_.root + '/' + path;

        int fd = open(path.c_str(), O_RDONLY);
        if (fd >= 0) {
            std::string body;
            char buf[4096];
            ssize_t n;

            while ((n = read(fd, buf, sizeof(buf))) > 0)
                body.append(buf, n);

            close(fd);

            res.contentLength = toString(body.size());

            std::string header = generateHeader(res);
            conn.sendBuffer.append(header);
            conn.sendBuffer.append(body);

            modifyToWrite(conn.fd);
            return;
        }
		log(WARNING, "Error page " + path + " not found");
    }

    page = generateDefaultPage(code, &pageSize);

    res.contentLength = toString(pageSize);

    std::string header = generateHeader(res);

    conn.sendBuffer.append(header);
    conn.sendBuffer.append(page, pageSize);

    modifyToWrite(conn.fd);
}

