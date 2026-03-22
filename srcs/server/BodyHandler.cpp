#include "ConfigParser.h"
#include "defines.h"
#include "webserv.h"

static bool requestComplete(const Buffer& buf, size_t& endPos) {
    const char* data = buf.data();
    size_t len = buf.size();

    for (size_t i = 0; i + 3 < len; ++i) {
        if (data[i] == '\r' &&
            data[i+1] == '\n' &&
            data[i+2] == '\r' &&
            data[i+3] == '\n')
        {
            endPos = i + 4;
            return true;
        }
    }
    return false;
}

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

static int checkMethod(const Request& req, const LocationConfig& location) {
    const std::string& method = req.method;

    if (location.methods.empty()) {
        if (method == "GET" ||
            method == "POST" ||
            method == "DELETE")
            return OK;

        return BAD_REQUEST;
    }

    for (std::vector<std::string>::const_iterator it = location.methods.begin();
         it != location.methods.end();
         ++it)
    {
        if (*it == method)
            return OK;
    }

    return METHOD_NOT_ALLOWED;
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
    req.bodySent = 0;
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

    if (req.bodySource == BODY_FROM_MEMORY) {
        if (req.body.size() + size <= MAX_MEMORY_BODY_SIZE) {
            req.body.append(data, size);
            return;
        }

        req.bodySource = BODY_FROM_FILE;
        req.fileBuffer = openTempFile();

        if (!writeToFile(req.fileBuffer,
                    req.body.c_str(),
                    req.body.size()))
            return false;

        req.body.clear();
    }

    return writeToFile(req.fileBuffer, data, size);
}

void Server::startBodyReading(Connection& conn) {
    Request& req = conn.req;
    LocationConfig& loc = conn.location;

    bool chunked = isChunked(req);
    bool hasCL = (req.headers.find("Content-Length") != req.headers.end());

    if (chunked && hasCL)
        return sendError(BAD_REQUEST, conn);

    if (chunked) {
        conn.state = READING_CHUNK_SIZE;
        conn.req.transferType = CHUNKED;
        return;
    }

    conn.req.transferType = FIXED;
    if (parseContentLength(req)) {

        if (loc.hasClientMaxBodySize && req.bodySize > loc.clientMaxBodySize)
            return sendError(PAYLOAD_TOO_LARGE, conn);

        if (req.bodySize > MAX_MEMORY_BODY)
            conn.req.bodySource = BODT_FROM_FILE;
        else
            conn.req.bodySource = BODY_FROM_MEMORY;
        conn.state = READING_BODY;

        std::string cgiPath = findCGI(req.path, loc.cgi);
        if (!cgiPath.empty())
            return runCGI(cgiPath, conn);
        return;
    }

    return sendError(BAD_REQUEST, conn);
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
        // DONE BODY READING
    }
}

void Server::processChunkedBody(Connection& conn) {
    Request& req = conn.req;

    while (true) {
        if (!req.hasChunkSize) {
            size_t pos;

            if (!conn.recvBuffer.findCRLF(pos)) {
                if (conn.recvBuffer.size() > MAX_CHUNK_SIZE_LEN)
                    return sendError(BAD_REQUEST, conn);
                return;
            }

            if (pos > MAX_CHUNK_SIZE_LEN)
                return sendError(BAD_REQUEST, conn);

            std::string line(conn.recvBuffer.data(), pos);

            req.chunkSize = std::strtoul(line.c_str(), NULL, 16);

            conn.recvBuffer.consume(pos + 2);
            req.hasChunkSize = true;

            if (req.chunkSize == 0) {
                return;
            }
        }

        if (conn.recvBuffer.size() < req.chunkSize + 2)
            return;

        if (!appendToBody(conn,
                          conn.recvBuffer.data(),
                          req.chunkSize))
        {
            return sendError(SERVER_ERROR, conn);
        }

        const char* tail = conn.recvBuffer.data() + req.chunkSize;

        if (tail[0] != '\r' || tail[1] != '\n')
            return sendError(BAD_REQUEST, conn);

        conn.recvBuffer.consume(req.chunkSize + 2);

        req.hasChunkSize = false;
    }
}

void Server::processBody(Connection& conn) {
    if (conn.req.transferType == FIXED)
        processFixedBody(conn);
    else
        processChunkedBody(conn);
}
