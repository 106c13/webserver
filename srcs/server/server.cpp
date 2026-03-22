#include <unistd.h>  
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <ctime>
#include <iostream>
#include <exception>
#include <cerrno>
#include <cstring>
#include <sstream>
#include "webserv.h"
#include "ConfigParser.h"

void Server::processHeaders(Connection& conn) {
    size_t endPos;

    if (!requestComplete(conn.recvBuffer, endPos)) {
        if (conn.recvBuffer.size() > MAX_HEADER_SIZE)
            return sendError(BAD_REQUEST, conn);
        return;
    }

    if (endPos > MAX_HEADER_SIZE)
        return sendError(BAD_REQUEST, conn);

    conn.req.header.assign(conn.recvBuffer.data(), endPos);
    conn.recvBuffer.consume(endPos);

    std::cout << conn.req.header;
    parser_.parse(conn.req.header);
    conn.req = parser_.getRequest();

    conn.location = resolveLocation(conn.req.path);

    log(INFO, conn.req.version + " " + conn.req.method + " " + conn.req.uri);

    int status;
    
    status = checkMethod(conn.req, con.location);
    if (status != OK)
        return sendError(status, conn);

    status = resolvePath(conn.req.path, con.location);
    if (status == DIRECTORY_NO_INDEX && conn.location.autoindex)
        return generateAutoindex(conn);

    if (status != OK)
        return sendError(status, conn);
    
    if (location.redirectCode != 0)
        return sendRedirect(conn);

    if (conn.req.method == "GET")
        return handleGet(conn);

    if (conn.req.method == "DELETE")
        return sendError(deleteResource(req.path), conn);

    return startBodyReading(conn);
}


//====================================================

static void setupConnection(Connection& conn, int fd) {
	conn.fd = fd;
    conn.sendingFile = false;
    conn.state = READING_HEADERS;
    conn.lastActivityTime = std::time(NULL);
}

void Server::acceptConnection() {
    sockaddr_in addr;
    socklen_t len = sizeof(addr);

    while (true) {
        int clientFd = accept(serverFd_, (sockaddr*)&addr, &len);
		if (clientFd < 0) {
    		return;
		}

        fcntl(clientFd, F_SETFL, O_NONBLOCK);

#ifdef __linux__
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = clientFd;
        epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev);
#elif __APPLE__
        struct kevent ev;
        EV_SET(&ev, clientFd, EVFILT_READ, EV_ADD, 0, 0, NULL);
        kevent(epollFd_, &ev, 1, NULL, 0, NULL);
#endif
        setupConnection(connections_[clientFd], clientFd);
        std::cout << "New connection..." << std::endl;
    }
}

void Server::initSocket() {
	std::ostringstream oss;

	log(INFO, "Starting server");
	serverFd_ = socket(AF_INET, SOCK_STREAM, 0);

	if (serverFd_ < 0) {
		throw std::runtime_error("socket() failed");
	}

	int opt = 1;
	if (setsockopt(serverFd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		throw std::runtime_error("setsockopt() failed");
	}

	addr_ = sockaddr_in();
	addr_.sin_family = AF_INET;
	addr_.sin_addr.s_addr = INADDR_ANY;
	addr_.sin_port = htons(config_.port);

	if (bind(serverFd_, (sockaddr*)&addr_, sizeof(addr_)) < 0) {
		oss << "bind() failed on port " << config_.port;
		throw std::runtime_error(oss.str());
	}

	if (listen(serverFd_, 99) < 0) {
		oss << "listen() failed on port " << config_.port;
		throw std::runtime_error(oss.str());
	}
	
	fcntl(serverFd_, F_SETFL, O_NONBLOCK);

	oss << "Server started on port ";
	oss << config_.port;
	log(INFO, oss.str()); 
}

void Server::checkCGIProcesses() {
    std::vector<CGIProcess>::iterator it = cgiProcesses_.begin();

    while (it != cgiProcesses_.end()) {
        int status;
        pid_t result = waitpid(it->pid, &status, WNOHANG);

        if (result > 0) {
            handleCGIRead(*(it->conn));
            it = cgiProcesses_.erase(it);
        } else {
            ++it;
        }
    }
}

void Server::loop() {
#ifdef __linux__
    epoll_event events[1024];
#elif __APPLE__
    struct kevent events[1024];
    struct timespec timeout;
    
    timeout.tv_sec = 1;
    timeout.tv_nsec = 0;
#endif

    while (true) {
#ifdef __linux__
        int evCount = epoll_wait(epollFd_, events, 1024, 1000);
#elif __APPLE__
        int evCount = kevent(epollFd_, NULL, 0, events, 1024, &timeout);
#endif

        checkTimeOuts();
        checkCGIProcesses();
        startQueuedCGIs();
        if (evCount < 0)
            continue;

        for (int i = 0; i < evCount; i++) {

            int fd;
#ifdef __linux__
            fd = events[i].data.fd;
#elif __APPLE__
            fd = (int)events[i].ident;
#endif

            Event& ev = events[i];

            // -------- CGI PIPE HANDLING --------
            std::map<int, Connection*>::iterator it = cgiFdMap_.find(fd);
            if (it != cgiFdMap_.end()) {
                Connection* conn = it->second;
#ifdef __linux__
                if (events[i].events & EPOLLOUT)
                    handleCGIWrite(*conn);

                if (events[i].events & EPOLLIN)
                    handleCGIRead(*conn);
#elif __APPLE__
                if (events[i].filter == EVFILT_WRITE)
                    handleCGIWrite(*conn);

                if (events[i].filter == EVFILT_READ)
                    handleCGIRead(*conn);
#endif
                continue;
            }
            // -----------------------------------

            if (fd == serverFd_) {
                acceptConnection();
            } else {
                handleClient(ev);
            }
        }
    }
}

Server::Server(const ServerConfig& config) {
#ifdef __linux__
	epollFd_ = epoll_create(1024);
#elif __APPLE__
	epollFd_ = kqueue();
#endif
	config_ = config;
	
	initSocket();
	
	fcntl(serverFd_, F_SETFL, O_NONBLOCK);

#ifdef __linux__
    epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = serverFd_;
    epoll_ctl(epollFd_, EPOLL_CTL_ADD, serverFd_, &ev);
#elif __APPLE__
    struct kevent ev;
    EV_SET(&ev, serverFd_, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(epollFd_, &ev, 1, NULL, 0, NULL);
#endif
}

Server::~Server() {
	if (serverFd_ >= 0) {
		close(serverFd_);
	}
}
