#include <fcntl.h>
#include <cstdio>
#include "webserv.h"
#include "ConfigParser.h"

int deleteResource(const std::string& path) {
    if (access(path.c_str(), W_OK) != 0)
        return FORBIDDEN;

    if (remove(path.c_str()) != 0)
        return SERVER_ERROR;

    return NO_CONTENT;
}


void Server::handleRequest(Connection& conn) {
    Request& req = conn.req;
    Response& res = conn.res;
    
    LocationConfig location = resolveLocation(req.path);
    resolvePath(req.path, location);

    res.path = req.path;
	
	if (req.method == "DELETE") {
		return sendError(deleteResource(req.path), conn);
	}
	
	std::string cgiPath = findCGI(req.path, location.cgi);

	if (!cgiPath.empty()) {
		int fd = runCGI(cgiPath.c_str(), conn);
		if (fd < 0) {
			return sendError(SERVER_ERROR, conn);
        }
        return sendCGIOutput(conn, fd);
	}
	
	if (req.method == "POST") {
        std::string contentType = req.headers["Content-Type"];

        if (contentType.find("multipart/form-data") != std::string::npos) {
            StringMap formFields;

            for (std::vector<MultipartPart>::iterator it = req.multipartParts.begin();
                 it != req.multipartParts.end();
                 ++it)
            {
                if (!it->filename.empty()) {
                    std::string uploadDir = location.uploadDir;
                    std::string safeName = it->filename;

                    size_t slash = safeName.find_last_of("/\\");
                    if (slash != std::string::npos)
                        safeName = safeName.substr(slash + 1);

                    std::string fullPath = uploadDir + "/" + safeName;

                    int fd = open(fullPath.c_str(),
                                  O_WRONLY | O_CREAT | O_TRUNC,
                                  0644);

                    if (fd < 0)
                        return sendError(SERVER_ERROR, conn);

                    ssize_t written = write(fd,
                                            it->data.data(),
                                            it->data.size());

                    close(fd);

                    if (written < 0)
                        return sendError(SERVER_ERROR, conn);
                } else {
                    formFields[it->name] = it->data;
                }
            }
        }
    }
	conn.res.status = OK;
    if (!prepareFileResponse(conn, req.path))
        return sendError(SERVER_ERROR, conn);

    streamFileChunk(conn);
	modifyToWrite(conn.fd);
}
