#include <fcntl.h>
#include <cstdio>
#include "webserv.h"
#include "ConfigParser.h"

bool Server::handleMultipartUpload(Connection& conn, LocationConfig& location) {
    for (std::vector<MultipartPart>::iterator it = conn.req.multipartParts.begin();
         it != conn.req.multipartParts.end();
         ++it)
    {
        if (it->filename.empty())
            continue;

        std::string safeName = it->filename;
        size_t slash = safeName.find_last_of("/\\");
        if (slash != std::string::npos)
            safeName = safeName.substr(slash + 1);

        std::string fullPath = location.uploadDir + "/" + safeName;

        int fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
            return false;

        ssize_t written = write(fd, it->data.data(), it->data.size());
        close(fd);

        if (written < 0)
            return false;
    }
    return true;
}


void Server::handlePost(Connection& conn) {
    Request& req = conn.req;
    Response& res = conn.res;
    LocationConfig& location = conn.location;

    res.path = req.path;
    conn.state = PROCESSING;
    std::cout << "In handlePost\n";
    std::string cgiPath = findCGI(req.path, location.cgi);
    if (!cgiPath.empty())
        return runCGI(cgiPath.c_str(), conn);

    if (!prepareFileResponse(conn, req.path))
        return sendError(SERVER_ERROR, conn);

    streamFileChunk(conn);
    modifyToWrite(conn.fd);
}

void Server::handleGet(Connection& conn) {
    Request& req = conn.req;
    Response& res = conn.res;
    LocationConfig& location = conn.location;

    if (location.root.empty())
        return sendError(OK, conn);

    res.path = req.path;

    std::string cgiPath = findCGI(req.path, location.cgi);
    if (!cgiPath.empty())
        return runCGI(cgiPath.c_str(), conn);

    if (!prepareFileResponse(conn, req.path))
        return sendError(SERVER_ERROR, conn);

    streamFileChunk(conn);
    modifyToWrite(conn.fd);
}
