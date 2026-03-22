#include <cerrno>
#include "webserv.h"
#include "Buffer.h"

void Server::handleRead(Connection& conn) {
    conn.lastActivityTime = std::time(NULL);
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
    conn.lastActivityTime = std::time(NULL);

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
        if (conn.sendBuffer.empty() && conn.fileFd < 0) {
            conn.state = CLOSED;
            return;
        }
    }

    if (conn.fileFd >= 0)
        streamFileChunk(conn);

    if (conn.sendBuffer.empty())
        modifyToRead(conn.fd);
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
    if (event.filter == EVFILT_READ)
        handleRead(conn);
    if (event.filter == EVFILT_WRITE)
        handleWrite(conn);
#endif

    if (conn.state == CLOSED)
        return closeConnection(conn.fd);

    if (conn.state == READING_HEADER)
        processHeaders(conn);

    if (conn.state == READING_BODY
        || conn.state == READING_CHUNK_SIZE
        || conn.state == READING_CHUNK_DATA
        || conn.state == READING_CHUNK_CRLF)
        processBody(conn);
}
