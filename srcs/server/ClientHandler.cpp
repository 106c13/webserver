#include <cerrno>
#include "webserv.h"
#include "Buffer.h"

static bool requestComplete(const Buffer& buf, size_t& endPos)
{
    const char* data = buf.data();
    size_t len = buf.size();

    for (size_t i = 0; i + 3 < len; ++i) {
        if (data[i] == '\r' &&
            data[i+1] == '\n' &&
            data[i+2] == '\r' &&
            data[i+3] == '\n')
        {
            endPos = i + 4; // save the end position
            return true;
        }
    }
    return false;
}

void Server::handleRead(Connection& conn) {
    char buf[4096];

    while (true) {
        ssize_t n = recv(conn.fd, buf, sizeof(buf), 0);

        if (n > 0)
            conn.recvBuffer.append(buf, n);
        else if (n == 0)
            return closeConnection(conn.fd);
        else
            break;
    }

    if (conn.state == READING_HEADERS) {
        size_t endPos;

        if (!requestComplete(conn.recvBuffer, endPos))
            return;

        std::string raw(conn.recvBuffer.data(), endPos);


        parser_.parse(raw);
        conn.req = parser_.getRequest();

        conn.recvBuffer.consume(endPos);

        conn.state = PROCESSING;

        if (conn.req.method == "GET") {
            handleRequest(conn);
            return;
        }

        if (conn.req.method == "POST") {
            if (conn.req.headers.find("Content-Length") == conn.req.headers.end())
                return sendError(LENGTH_REQUIRED, conn);

            conn.remainingBody = conn.req.contentLenght;

            //if (conn.remainingBody > conn.configMaxBodySize)
            //    return sendError(PAYLOAD_TOO_LARGE, conn);

            conn.state = READING_BODY;
        } else {
            return sendError(METHOD_NOT_ALLOWED, conn);
        }
    }

    if (conn.state == READING_BODY) {
        size_t available = conn.recvBuffer.size();

        if (available == 0)
            return;

        size_t toConsume = std::min(available, conn.remainingBody);

        conn.req.body.append(conn.recvBuffer.data(), toConsume);

        conn.recvBuffer.consume(toConsume);
        conn.remainingBody -= toConsume;

        if (conn.remainingBody == 0) {
            conn.state = PROCESSING;
            handleRequest(conn);
        }
    }

    if (!conn.sendBuffer.empty())
        modifyToWrite(conn.fd);
}

void Server::handleWrite(Connection& conn) {
    while (!conn.sendBuffer.empty()) {
        ssize_t n = send(conn.fd,
                         conn.sendBuffer.data(),
                         conn.sendBuffer.size(),
                         0);
        if (n > 0) {
            conn.sendBuffer.consume(n);
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            closeConnection(conn.fd);
            return;
        }
    }
    
    if (conn.sendingFile) {
        streamFileChunk(conn);
    }

    if (conn.sendBuffer.empty()) {
        modifyToRead(conn.fd);
    }
}
