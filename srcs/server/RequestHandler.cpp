#include <cstdio>
#include <fcntl.h>
#include "webserv.h"
#include "ConfigParser.h"

static std::string extractFilename(const std::string& headers) {
    size_t fpos = headers.find("filename=\"");
    if (fpos == std::string::npos)
        return "";

    fpos += 10;
    size_t fend = headers.find("\"", fpos);
    if (fend == std::string::npos)
        return "";

    std::string name = headers.substr(fpos, fend - fpos);

    size_t slash = name.find_last_of("/\\");
    if (slash != std::string::npos)
        name = name.substr(slash + 1);

    return name;
}

static bool handleMultipart(Connection& conn) {
    Request& req = conn.req;
    LocationConfig& location = conn.location;

    std::string boundary = "--" + req.boundary;

    int fd = open(req.tempFilePath.c_str(), O_RDONLY);
    if (fd < 0)
        return false;

    char buf[BUFFER_SIZE];
    std::string buffer;
    ssize_t n;

    int out_fd = -1;
    bool in_file = false;

    while ((n = read(fd, buf, BUFFER_SIZE)) > 0) {
        buffer.append(buf, n);

        size_t pos;

        while ((pos = buffer.find(boundary)) != std::string::npos) {

            if (in_file && out_fd != -1 && pos > 0) {
                write(out_fd, buffer.data(), pos);
            }

            buffer.erase(0, pos + boundary.size());

            // end of multipart
            if (buffer.compare(0, 2, "--") == 0)
                break;

            // skip CRLF
            if (buffer.compare(0, 2, "\r\n") == 0)
                buffer.erase(0, 2);

            // close previous file
            if (out_fd != -1) {
                close(out_fd);
                out_fd = -1;
            }

            // parse headers
            size_t header_end;
            while ((header_end = buffer.find("\r\n\r\n")) == std::string::npos) {
                n = read(fd, buf, BUFFER_SIZE);
                if (n <= 0) break;
                buffer.append(buf, n);
            }

            if (header_end == std::string::npos)
                break;

            std::string headers = buffer.substr(0, header_end);
            buffer.erase(0, header_end + 4);

            std::string filename = extractFilename(headers);

            if (!filename.empty()) {
                std::string fullPath = location.uploadDir + "/" + filename;

                out_fd = open(fullPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (out_fd < 0) {
                    close(fd);
                    return false;
                }
                in_file = true;
            } else {
                in_file = false;
            }
        }

        // write safe chunk (avoid cutting boundary)
        if (in_file && out_fd != -1) {
            if (buffer.size() > boundary.size()) {
                size_t safe = buffer.size() - boundary.size();
                write(out_fd, buffer.data(), safe);
                buffer.erase(0, safe);
            }
        }
    }

    if (out_fd != -1)
        close(out_fd);

    close(fd);
    return true;
}


void ServerManager::handlePost(Connection& conn) {
    Request& req = conn.req;
    Response& res = conn.res;
    LocationConfig& location = conn.location;

    res.path = req.path;
    conn.state = PROCESSING;

    if (location.root.empty())
        return sendError(OK, conn);

    std::string cgiPath = findCGI(req.path, conn.config->cgi);
    if (cgiPath.empty())
        cgiPath = findCGI(req.path, location.cgi);
    if (!cgiPath.empty())
        return runCGI(cgiPath.c_str(), conn);

    handleMultipart(conn);

    if (!prepareFileResponse(conn, req.path))
        return sendError(SERVER_ERROR, conn);

    streamFileChunk(conn);
    modifyToWrite(conn.fd);
}

void ServerManager::handleGet(Connection& conn) {
    Request& req = conn.req;
    Response& res = conn.res;
    LocationConfig& location = conn.location;

    if (location.root.empty())
        return sendError(OK, conn);

    res.path = req.path;

    std::string cgiPath = findCGI(req.path, conn.config->cgi);
    if (cgiPath.empty())
        cgiPath = findCGI(req.path, location.cgi);
        
    if (!cgiPath.empty())
        return runCGI(cgiPath.c_str(), conn);

    if (!prepareFileResponse(conn, req.path))
        return sendError(SERVER_ERROR, conn);

    streamFileChunk(conn);
    modifyToWrite(conn.fd);
}
