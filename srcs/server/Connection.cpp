#include <fcntl.h>
#include <ctime>
#include "webserv.h"
#include "ConfigParser.h"

void Server::checkTimeOuts() {
    for (std::map<int, Connection>::iterator it = connections_.begin(); 
         it != connections_.end(); )
    {
        Connection& conn = it->second;

        size_t diff = std::time(NULL) - conn.lastActivityTime;

        if ((conn.state == READING_HEADERS && diff > HEADER_TIMEOUT) ||
            ((conn.state == READING_BODY || conn.state == READING_CHUNKS) && diff > BODY_TIMEOUT)) 
        {
            conn.state = CLOSED;
            sendError(REQUEST_TIMEOUT, conn);
            std::map<int, Connection>::iterator toErase = it++;
        } else
            ++it;
    }
}

void Server::modifyToWrite(int fd) {
#ifdef __linux__
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLOUT;
    ev.data.fd = fd;
    epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
#elif __APPLE__
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(epollFd_, &ev, 1, NULL, 0, NULL);
    EV_SET(&ev, fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
    kevent(epollFd_, &ev, 1, NULL, 0, NULL);
#endif
}

void Server::modifyToRead(int fd) {
#ifdef __linux__
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
#elif __APPLE__
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    kevent(epollFd_, &ev, 1, NULL, 0, NULL);
    EV_SET(&ev, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(epollFd_, &ev, 1, NULL, 0, NULL);
#endif
}

void Server::closeConnection(int fd) {
#ifdef __linux__
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, NULL);
#elif __APPLE__
    struct kevent ev;
    EV_SET(&ev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    kevent(epollFd_, &ev, 1, NULL, 0, NULL);
    EV_SET(&ev, fd, EVFILT_WRITE, EV_DELETE, 0, 0, NULL);
    kevent(epollFd_, &ev, 1, NULL, 0, NULL);
#endif
    // Graceful close: signal we're done writing, then drain any
    // remaining data so close() sends FIN instead of RST.
    shutdown(fd, SHUT_WR);
    char drain[4096];
    while (recv(fd, drain, sizeof(drain), 0) > 0)
        ;
    close(fd);
    connections_.erase(fd);
    std::cout << "Client disconnected..." << std::endl;
}
