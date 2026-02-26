#include <fcntl.h>
#include "webserv.h"
#include "ConfigParser.h"

static int checkRequest(const Request& request, const LocationConfig& location) {
	if (location.methods.empty()) {
        return OK;
	}

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
    
    LocationConfig location = resolveLocation(req.path);

    int status = checkRequest(req, location);
    if (status != OK)
        return sendError(status, conn);

    if (location.redirectCode != 0)
        return sendRedirect(conn, location);

    status = resolvePath(req.path, location);

    if (status == DIRECTORY_NO_INDEX && location.autoindex)
        return generateAutoindex(conn, location);
	else if (status != OK)
        return sendError(status, conn);

    res.path = req.path;

    if (req.method == "POST") {
        std::string contentType = req.headers["Content-Type"];

        if (contentType.find("multipart/form-data") != std::string::npos) {
            std::map<std::string, std::string> formFields;

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
                    std::cout << fullPath << std::endl;

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
