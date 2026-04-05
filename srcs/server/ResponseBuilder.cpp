#include <fcntl.h>
#include <string>
#include "webserv.h"
#include "ConfigParser.h"

static void flushHeader(Connection& conn) {
    std::string header = generateHeader(conn.res);
    conn.buffer.append(header);
}

void ServerManager::sendRedirect(Connection& conn) {
    Response& res = conn.res;
    res.status = conn.location.redirectCode;
    res.location = conn.location.redirectUrl;
    res.contentLength = "0";

    flushHeader(conn);
    modifyToWrite(conn.fd);
}

bool ServerManager::prepareFileResponse(Connection& conn, const std::string& path) {
    ssize_t size = getFileSize(path);
    if (size < 0)
        return false;

    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return false;

    Response& res = conn.res;
    res.path = path;
    res.contentLength = toString(size);
    res.connectionType = "close";


    conn.buffer.clear();
    flushHeader(conn);
    conn.fileBuffer = fd;
    conn.state = SENDING_FILE;
    return true;
}

bool ServerManager::streamFileChunk(Connection& conn) {
    if (conn.fileBuffer < 0)
        return false;

    char buf[BUFFER_SIZE * 4];
    ssize_t n = read(conn.fileBuffer, buf, sizeof(buf));

    if (n > 0) {
        conn.buffer.append(buf, n);
        return true;
    }

    close(conn.fileBuffer);
    conn.fileBuffer = -1;
    conn.state = SENDING_RESPONSE;
    return false;
}
