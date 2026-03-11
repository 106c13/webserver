#include <cerrno>
#include "webserv.h"
#include "Buffer.h"

void Server::handleRead(Connection& conn) {
    char buf[BUFFER_SIZE];

    while (true) {
        ssize_t n = recv(conn.fd, buf, sizeof(buf), 0);

        if (n > 0)
            conn.recvBuffer.append(buf, n);
        else if (n == 0)
            return closeConnection(conn.fd);
        else
            break;
        break;
    }
}

void Server::handleWrite(Connection& conn) {
    while (!conn.sendBuffer.empty()) {
        ssize_t n = send(conn.fd,
                         conn.sendBuffer.data(),
                         conn.sendBuffer.size(),
                         0);
        if (n > 0) {
            conn.sendBuffer.consume(n);
        } else if (n == 0) {
            return closeConnection(conn.fd);
        } else {
            break;
        }
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
