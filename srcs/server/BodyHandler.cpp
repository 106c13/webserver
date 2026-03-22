#include <fcntl.h>
#include <sstream>
#include "ConfigParser.h"
#include "defines.h"
#include "webserv.h"

static std::string trimStr(const std::string& str) {
    size_t start = str.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t");
    return str.substr(start, end - start + 1);
}

void parseMultipart(Request& req) {
    if (req.boundary.empty() || req.body.empty())
        return;

    req.multipartParts.clear();

    std::string delimiter = "--" + req.boundary;
    size_t pos = req.body.find(delimiter);
    if (pos == std::string::npos)
        return;
    pos += delimiter.size();

    size_t bodyLen = req.body.size();
    while (pos < bodyLen) {
        if (pos < bodyLen && req.body[pos] == '\r') pos++;
        if (pos < bodyLen && req.body[pos] == '\n') pos++;

        if (req.body.substr(pos, 2) == "--")
            break;

        MultipartPart part;

        while (pos < bodyLen) {
            size_t lineEnd = req.body.find("\r\n", pos);
            if (lineEnd == std::string::npos)
                lineEnd = req.body.find("\n", pos);
            if (lineEnd == std::string::npos)
                break;

            std::string line = req.body.substr(pos, lineEnd - pos);
            if (line.empty() || line == "\r") {
                pos = lineEnd + 1;
                if (pos > 0 && pos < bodyLen && req.body[pos - 1] == '\r')
                    pos++;
                break;
            }

            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string headerName = line.substr(0, colonPos);
                std::string headerValue = trimStr(line.substr(colonPos + 1));
                part.headers[headerName] = headerValue;

                if (headerName == "Content-Disposition") {
                    size_t namePos = headerValue.find("name=\"");
                    if (namePos != std::string::npos) {
                        namePos += 6;
                        size_t nameEnd = headerValue.find("\"", namePos);
                        if (nameEnd != std::string::npos)
                            part.name = headerValue.substr(namePos, nameEnd - namePos);
                    }
                    size_t filePos = headerValue.find("filename=\"");
                    if (filePos != std::string::npos) {
                        filePos += 10;
                        size_t fileEnd = headerValue.find("\"", filePos);
                        if (fileEnd != std::string::npos)
                            part.filename = headerValue.substr(filePos, fileEnd - filePos);
                    }
                }
                if (headerName == "Content-Type")
                    part.contentType = headerValue;
            }

            pos = lineEnd + 1;
            if (pos > 0 && pos < bodyLen && req.body[pos - 1] == '\r')
                pos++;
        }

        size_t nextDelim = req.body.find(delimiter, pos);
        if (nextDelim == std::string::npos) {
            part.data = req.body.substr(pos);
            req.multipartParts.push_back(part);
            break;
        }

        size_t dataEnd = nextDelim;
        if (dataEnd > 0 && req.body[dataEnd - 1] == '\n') dataEnd--;
        if (dataEnd > 0 && req.body[dataEnd - 1] == '\r') dataEnd--;

        part.data = req.body.substr(pos, dataEnd - pos);
        req.multipartParts.push_back(part);

        pos = nextDelim + delimiter.size();
    }
}

bool requestComplete(const Buffer& buf, size_t& endPos) {
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

int checkMethod(const Request& req, const LocationConfig& location) {
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
    req.bodyReceived = 0;
    return true;
}

static bool isChunked(const Request& req) {
    StringMap::const_iterator it = req.headers.find("Transfer-Encoding");
    return (it != req.headers.end() && it->second == "chunked");
}

static int openTempFile(std::string& outPath) {
    static int counter = 0;
    std::ostringstream oss;
    oss << "/tmp/webserv_body_" << counter++;
    outPath = oss.str();
    return open(outPath.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0600);
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
            return true;
        }

        req.bodySource = BODY_FROM_FILE;
        req.fileBuffer = openTempFile(req.tempFilePath);
        if (req.fileBuffer < 0)
            return false;

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
        req.transferType = CHUNKED;
        return;
    }

    req.transferType = FIXED;
    if (!parseContentLength(req))
        return sendError(BAD_REQUEST, conn);

    if (loc.hasClientMaxBodySize && req.bodySize > loc.clientMaxBodySize)
        return sendError(PAYLOAD_TOO_LARGE, conn);

    if (req.bodySize > MAX_MEMORY_BODY_SIZE)
        req.bodySource = BODY_FROM_FILE;
    else
        req.bodySource = BODY_FROM_MEMORY;

    std::string cgiPath = findCGI(req.path, loc.cgi);
    if (!cgiPath.empty()) {
        conn.state = CGI_WRITING_BODY;
        return runCGI(cgiPath.c_str(), conn);
    }

    conn.state = READING_BODY;
}

void Server::processFixedBody(Connection& conn) {
    Request& req = conn.req;

    size_t remaining = req.bodySize - req.bodyReceived;
    size_t available = conn.recvBuffer.size();
    size_t toRead = remaining < available ? remaining : available;

    if (!appendToBody(conn, conn.recvBuffer.data(), toRead))
        return sendError(SERVER_ERROR, conn);

    conn.recvBuffer.consume(toRead);
    req.bodyReceived += toRead;

    if (req.bodyReceived >= req.bodySize)
        finishBody(conn);
}

void Server::processChunkedBody(Connection& conn) {
    while (true) {
        if (!conn.hasChunkSize) {
            size_t pos;

            if (!conn.recvBuffer.findCRLF(pos)) {
                if (conn.recvBuffer.size() > MAX_CHUNK_SIZE_LEN)
                    return sendError(BAD_REQUEST, conn);
                return;
            }

            if (pos > MAX_CHUNK_SIZE_LEN)
                return sendError(BAD_REQUEST, conn);

            std::string line(conn.recvBuffer.data(), pos);

            conn.chunkSize = std::strtoul(line.c_str(), NULL, 16);
            conn.recvBuffer.consume(pos + 2);
            conn.hasChunkSize = true;

            if (conn.chunkSize == 0) {
                if (conn.recvBuffer.size() >= 2)
                    conn.recvBuffer.consume(2);
                return finishBody(conn);
            }
        }

        if (conn.recvBuffer.size() < conn.chunkSize + 2)
            return;

        if (!appendToBody(conn,
                          conn.recvBuffer.data(),
                          conn.chunkSize))
        {
            return sendError(SERVER_ERROR, conn);
        }

        const char* tail = conn.recvBuffer.data() + conn.chunkSize;

        if (tail[0] != '\r' || tail[1] != '\n')
            return sendError(BAD_REQUEST, conn);

        conn.recvBuffer.consume(conn.chunkSize + 2);
        conn.hasChunkSize = false;
    }
}

void Server::finishBody(Connection& conn) {
    Request& req = conn.req;
    LocationConfig& loc = conn.location;

    std::string cgiPath = findCGI(req.path, loc.cgi);
    if (!cgiPath.empty()) {
        if (req.bodySource == BODY_FROM_FILE) {
            close(req.fileBuffer);
            req.fileBuffer = open(req.tempFilePath.c_str(), O_RDONLY);
        }
        conn.state = CGI_WRITING_BODY;
        return runCGI(cgiPath.c_str(), conn);
    }

    conn.state = PROCESSING;

    if (!req.boundary.empty()) {
        parseMultipart(req);
        if (!handleMultipartUpload(conn, loc))
            return sendError(SERVER_ERROR, conn);
        return sendError(OK, conn);
    }

    sendError(OK, conn);
}

void Server::processBody(Connection& conn) {
    if (conn.req.transferType == FIXED)
        processFixedBody(conn);
    else
        processChunkedBody(conn);
}
