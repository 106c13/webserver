#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include "ConfigParser.h"
#include "defines.h"
#include "webserv.h"

static bool chunkedBodyComplete(const Buffer& buf, size_t& endPos) {
    const char* data = buf.data();
    size_t len = buf.size();

    if (len < 5)
        return false;


    for (size_t i = 0; i + 4 < len; ++i) {
        if (data[i] == '0' &&
            data[i+1] == '\r' &&
            data[i+2] == '\n' &&
            data[i+3] == '\r' &&
            data[i+4] == '\n')
        {
            endPos = i + 5;
            return true;
        }
    }
    return false;
}

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
    req.bodyReceived = 0;
    return true;
}

static bool isChunked(const Request& req) {
    StringMap::const_iterator it = req.headers.find("Transfer-Encoding");
    return (it != req.headers.end() && it->second == "chunked");
}

static bool writeToFile(int fd, const char* data, size_t size) {
    size_t written = 0;

    while (written < size) {
        ssize_t n = write(fd, data + written, size - written);

        if (n > 0)
            written += n;
        else if (n == -1)
            return false;
    }

    return true;
}

static bool appendToBody(Connection& conn, const char* data, size_t size) {
    Request& req = conn.req;

    req.bodyReceived += size;
    if (req.bodySource == BODY_FROM_MEMORY) {
        if (req.body.size() + size <= MAX_MEMORY_BODY_SIZE) {
            req.body.append(data, size);
            return true;
        }

        req.bodySource = BODY_FROM_FILE;
        req.fileBuffer = openTempFile();

        if (!writeToFile(req.fileBuffer,
                         req.body.c_str(),
                         req.body.size()))
            return false;

        req.body.clear();
    }

    if (!writeToFile(req.fileBuffer, data, size))
        return false;

    return true;
}

void Server::startBodyReading(Connection& conn) {
    Request& req = conn.req;
    LocationConfig& loc = conn.location;

    if (isChunked(req)) {
        conn.state = READING_CHUNK_SIZE;
        conn.req.transferType = CHUNKED;
        return;
    }

    if (!parseContentLength(req))
        return sendError(BAD_REQUEST, conn);

    if (loc.hasClientMaxBodySize && req.bodySize > loc.clientMaxBodySize)
        return sendError(PAYLOAD_TOO_LARGE, conn);

    if (req.bodySize > MAX_MEMORY_BODY_SIZE) {
        req.bodySource = BODY_FROM_FILE;
        req.fileBuffer = openTempFile();
    } else {
        req.bodySource = BODY_FROM_MEMORY;
    }

    conn.req.transferType = FIXED;
    conn.state = READING_BODY;
}

void Server::processFixedBody(Connection& conn) {
    Request& req = conn.req;

    size_t remaining = req.bodySize - req.bodyReceived;
    size_t available = conn.recvBuffer.size();

    size_t toRead = remaining < available ? remaining : available;

    if (appendToBody(conn, conn.recvBuffer.data(), toRead))
        return sendError(SERVER_ERROR, conn);

    conn.recvBuffer.consume(toRead);

    if (req.body.size() == req.bodySize) {
        req.bodySent = 0;
        std::string cgiPath = findCGI(req.path, conn.location.cgi);
        if (!cgiPath.empty())
            return runCGI(cgiPath.c_str(), conn);
    }

    // Handle request
}

void Server::processChunkedBody(Connection& conn) {
    conn.req.body.append("Data");
    conn.req.bodySize += 4;
}

void Server::processBody(Connection& conn) {
    if (conn.req.transferType == FIXED)
        processFixedBody(conn);
    else
        processChunkedBody(conn);
}

int openTempFile() {
    char tmpl[] = "/tmp/webserver_XXXXXX";

    int fd = mkstemp(tmpl);
    if (fd == -1)
        return -1;

    unlink(tmpl);
    return fd;
}
