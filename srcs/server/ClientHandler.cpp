#include <cerrno>
#include "webserv.h"
#include "Buffer.h"

void Server::handleRead(Connection& conn) {
    // When streaming body to CGI, cap the recv buffer to avoid
    // accumulating the entire request body in memory.
    size_t readSize;
    if (conn.state == CGI_WRITING_BODY || conn.state == CGI_BUFFERING_TO_FILE) {
        if (conn.recvBuffer.size() > BUFFER_SIZE * 16)
            return;
        readSize = BUFFER_SIZE * 16;
    } else {
        readSize = BUFFER_SIZE * 1000;
    }

    char buf[BUFFER_SIZE * 1000];
    ssize_t n = recv(conn.fd, buf, readSize, 0);

    if (n > 0)
        conn.recvBuffer.append(buf, n);
    else if (n == 0)
        return closeConnection(conn.fd);
}

void Server::handleWrite(Connection& conn) {
    ssize_t n = send(conn.fd,
                     conn.sendBuffer.data(),
                     conn.sendBuffer.size(),
                     0);
    if (n > 0) {
        conn.sendBuffer.consume(n);
    } else if (n == 0) {
        return closeConnection(conn.fd);
    }

    if (conn.state == SENDING_RESPONSE) {
        if (conn.sendBuffer.empty() && !conn.sendingFile) {
            conn.state = CLOSED;
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
