#include <cerrno>
#include <ctime>
#include "webserv.h"
#include "Buffer.h"

void ServerManager::handleRead(Connection& conn) {
    conn.lastActivityTime = std::time(NULL);

    char buf[BUFFER_SIZE * 1000];
    ssize_t n = recv(conn.fd, buf, sizeof(buf), 0);

    if (n > 0)
        conn.buffer.append(buf, n);
    else if (n == 0)
        conn.state = CLOSED;
}

void ServerManager::handleWrite(Connection& conn) {
    conn.lastActivityTime = std::time(NULL);
    ssize_t n = send(conn.fd,
                     conn.buffer.data(),
                     conn.buffer.size(),
                     0);

    if (n > 0) {
        conn.buffer.consume(n);
    } else if (n <= 0) {
        conn.state = CLOSED;
        return;
    }

    if (conn.state == SENDING_FILE) {
        streamFileChunk(conn);
    }

	if (conn.state == FINISHED && conn.buffer.empty())
		conn.state = CLOSED;

    if (conn.buffer.empty()) {
        modifyToRead(conn.fd);
    }
}

void ServerManager::handleClient(Event& event) {
    int fd;
#ifdef __linux__
    fd = event.data.fd;
#elif __APPLE__
    fd = (int)event.ident;
#endif
    if (connections_.find(fd) == connections_.end())
        return;

    Connection& conn = connections_[fd];

    if (IS_EVENT_READ(event))
        handleRead(conn);

    if (conn.state == CLOSED)
        return closeConnection(conn.fd);

    if ((conn.state == SENDING_FILE || conn.state == SENDING_RESPONSE || conn.state == FINISHED) && IS_EVENT_WRITE(event))
        handleWrite(conn);

    if (conn.state == CLOSED)
        return closeConnection(conn.fd);

    if (conn.state == READING_HEADER)
        processHeaders(conn);

    if (conn.state == READING_BODY)
        processBody(conn);
}
