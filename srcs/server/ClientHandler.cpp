#include <cerrno>
#include "webserv.h"
#include "Buffer.h"

void Server::handleRead(Connection& conn) {
    conn.lastActivityTime = std::time(NULL);

    char buf[BUFFER_SIZE * 1000];
    ssize_t n = recv(conn.fd, buf, sizeof(buf), 0);

    if (n > 0)
        conn.recvBuffer.append(buf, n);
    else if (n == 0)
        return closeConnection(conn.fd);
}

void Server::handleWrite(Connection& conn) {
    conn.lastActivityTime = std::time(NULL);
    log(ERROR, "IN WRITE");
    ssize_t n = send(conn.fd,
                     conn.sendBuffer.data(),
                     conn.sendBuffer.size(),
                     0);
    std::cout << "N = " << n << std::endl;
    std::cout << "State = " << conn.state << std::endl;
    if (n > 0) {
        conn.sendBuffer.consume(n);
    } else if (n == 0) {
        return closeConnection(conn.fd);
    }

    if (conn.state == SENDING_FILE) {
        streamFileChunk(conn);
    }
    
    if (conn.sendBuffer.empty()) {
        modifyToRead(conn.fd);
    }
    log(ERROR, "Exited from write");
}

void Server::handleClient(Event& event) {
    int fd;
#ifdef __linux__
    fd = event.data.fd;
#elif __APPLE__
    fd = (int)event.ident;
#endif
    Connection& conn = connections_[fd];

#ifdef __linux__
    if (event.events & EPOLLIN)
        handleRead(conn);

    if (event.events & EPOLLOUT)
        handleWrite(conn);
#elif __APPLE__
    if (event.events & EVFILT_READ)
        handleRead(conn);

    if (event.events & EVFILT_WRITE)
        handleWrite(conn);
#endif

    if (conn.state == CLOSED)
        return closeConnection(conn.fd);

    if (conn.state == READING_HEADER)
        processHeaders(conn);

    if (conn.state == READING_BODY)
        processBody(conn);
}
