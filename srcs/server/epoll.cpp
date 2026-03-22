#include "webserv.h"

void Server::addEvent(int fd, bool wantRead, bool wantWrite)
{
#ifdef __linux__
    struct epoll_event ev;
    ev.events = 0;

    if (wantRead)
        ev.events |= EPOLLIN;
    if (wantWrite)
        ev.events |= EPOLLOUT;

    ev.data.fd = fd;

    epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);

#elif __APPLE__
    struct kevent ev[2];
    int n = 0;

    if (wantRead) {
        EV_SET(&ev[n++], fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    }

    if (wantWrite) {
        EV_SET(&ev[n++], fd, EVFILT_WRITE, EV_ADD, 0, 0, NULL);
    }

    kevent(epollFd_, ev, n, NULL, 0, NULL);
#endif
}
