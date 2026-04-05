#include <fcntl.h>
#include <unistd.h>
#include "ConfigParser.h"
#include "defines.h"
#include "webserv.h"

static bool parseContentLength(Request& req) {
    StringMap::iterator it = req.headers.find("Content-Length");
    if (it == req.headers.end())
        return false;

    char* end;
    unsigned long len = std::strtoul(it->second.c_str(), &end, 10);

    if (*end != '\0')
        return false;

    if (len > static_cast<unsigned long>(SIZE_MAX))
        return false;

    req.bodySize = static_cast<size_t>(len);
    return true;
}

static bool isChunked(const Request& req) {
    StringMap::const_iterator it = req.headers.find("Transfer-Encoding");
    return (it != req.headers.end() && it->second == "chunked");
}

void Server::startBodyReading(Connection& conn) {
    Request& req = conn.req;
    LocationConfig& loc = conn.location;

    req.bodySize = 0;
    req.bodyReceived = 0;
    req.bodySent = 0;
    
    req.fileBuffer = openTempFile(req.tempFilePath);

    if (isChunked(req)) {
        req.transferType = CHUNKED;
        conn.state = READING_BODY;
        return;
    }

    if (req.fileBuffer < 0)
        return sendError(SERVER_ERROR, conn);

    if (!parseContentLength(req))
        return sendError(BAD_REQUEST, conn);

    if (loc.hasClientMaxBodySize && req.bodySize > loc.clientMaxBodySize)
        return sendError(PAYLOAD_TOO_LARGE, conn);


    req.transferType = FIXED;
    conn.state = READING_BODY;
}

void Server::processFixedBody(Connection& conn) {
    Request& req = conn.req;
    Buffer& buf = conn.buffer;

    size_t remaining = req.bodySize - req.bodyReceived;
    size_t available = buf.size();
    size_t toWrite = std::min(remaining, available);

    ssize_t n = write(req.fileBuffer, buf.data(), toWrite);
    if (n <= 0)
        return sendError(SERVER_ERROR, conn);

    req.bodyReceived += n;
    buf.consume(n);

    if (req.bodyReceived == req.bodySize) {
        close(req.fileBuffer);
        req.fileBuffer = -1;
        handlePost(conn);
    }
}

void Server::processChunkedBody(Connection& conn) {
    Request& req = conn.req;
    Buffer& buf = conn.buffer;
    LocationConfig& location = conn.location;

    while (true) {
        if (!conn.hasChunkSize) {
            size_t pos;
            if (!buf.findCRLF(pos))
                return;

            std::string line(buf.data(), pos);
            conn.chunkSize = std::strtoul(line.c_str(), NULL, 16);
            buf.consume(pos + 2);
            conn.hasChunkSize = true;

            if (conn.chunkSize == 0) {
                if (req.fileBuffer != -1) {
                    close(req.fileBuffer);
                    req.fileBuffer = -1;
                }
                req.bodySize = req.bodyReceived;
                handlePost(conn);
                return;
            }
        }

        if (buf.size() < conn.chunkSize + 2)
            return;

        ssize_t n = write(req.fileBuffer, buf.data(), conn.chunkSize);
        if (n <= 0)
            return sendError(SERVER_ERROR, conn);

        req.bodyReceived += n;
        buf.consume(conn.chunkSize);

        if (location.hasClientMaxBodySize && location.clientMaxBodySize < req.bodyReceived)
            return sendError(PAYLOAD_TOO_LARGE, conn);

        if (buf.size() < 2)
            return;

        if (buf.data()[0] != '\r' || buf.data()[1] != '\n')
            return sendError(BAD_REQUEST, conn);

        buf.consume(2);
        conn.hasChunkSize = false;
    }
}

void Server::processBody(Connection& conn) {
    if (conn.req.transferType == FIXED)
        processFixedBody(conn);
    else
        processChunkedBody(conn);
}

int openTempFile(std::string& path) {
    int fd = -1;
    
    for (int i = 0; i < 10; ++i) {
        path = generateRandomName("/tmp/webserver_");
        fd = open(path.c_str(), O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd != -1)
            break;
    }
    
    return fd;
}
