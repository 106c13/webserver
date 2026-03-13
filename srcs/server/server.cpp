#include <unistd.h>  
#include <fcntl.h>
#include <sys/socket.h>
#include <ctime>
#include <iostream>
#include <exception>
#include <cstring>
#include <sstream>
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
            //closeConnection(toErase->first);
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
    close(fd);
    connections_.erase(fd);
    std::cout << "Client disconnected..." << std::endl;
}

static bool requestComplete(const Buffer& buf, size_t& endPos) {
    const char* data = buf.data();
    size_t len = buf.size();

    for (size_t i = 0; i + 3 < len; ++i) {
        if (data[i] == '\r' &&
            data[i+1] == '\n' &&
            data[i+2] == '\r' &&
            data[i+3] == '\n')
        {
            endPos = i + 4;
            return true;
        }
    }
    return false;
}

static bool chunkedBodyComplete(const Buffer& buf, size_t& endPos) {
    const char* data = buf.data();
    size_t len = buf.size();

    if (len < 5)
        return false;


    for (size_t i = 0; i + 4 < len; ++i) {
        if (data[i] == '0' &&
            data[i+1] == '\r' &&
            data[i+2] == '\n' &&
            data[i+3] == '\r' &&
            data[i+4] == '\n')
        {
            endPos = i + 5;
            return true;
        }
    }
    return false;
}

static int checkRequest(const Request& req, const LocationConfig& location) {
	if (location.methods.empty()) {
        return OK;
	}

    for (std::vector<std::string>::const_iterator it = location.methods.begin();
         it != location.methods.end();
         ++it) {
        if (*it == req.method) {
            return OK;
        }
    }

	std::string cgiPath = findCGI(req.path, location.cgi);
    std::cout << "TEST: ======================== " << cgiPath << std::endl;
    if (!cgiPath.empty())
        return OK;

    return METHOD_NOT_ALLOWED;
}

// ==================================

void Server::handleClient(Event& event) {
    int fd;
#ifdef __linux__
    fd = event.data.fd;
#elif __APPLE__
    fd = (int)event.ident;
#endif
    Connection& conn = connections_[fd];

#ifdef __linux__
    if (event.events & EPOLLIN) {
        conn.lastActivityTime = std::time(NULL);
        handleRead(conn);
    }

    if (event.events & EPOLLOUT) {
        conn.lastActivityTime = std::time(NULL);
        handleWrite(conn);
    }
#elif __APPLE__
    if (event.filter == EVFILT_READ) {
        conn.lastActivityTime = std::time(NULL);
        handleRead(conn);
    }

    if (event.filter & EVFILT_WRITE) {
        conn.lastActivityTime = std::time(NULL);
        handleWrite(conn);
    }
#endif

    if (conn.state == CLOSED)
        return closeConnection(conn.fd);

    if (conn.state == READING_HEADERS)
        processHeaders(conn);

    if (conn.state == READING_BODY ||
        conn.state == READING_CHUNKS ||
        conn.state == READING_CHUNK_SIZE ||
        conn.state == READING_CHUNK_DATA ||
        conn.state == READING_CHUNK_CRLF)
        processBody(conn);
}

void Server::processHeaders(Connection& conn) {
    size_t endPos;

    if (!requestComplete(conn.recvBuffer, endPos)) {
        if (conn.recvBuffer.size() > MAX_HEADER_SIZE)
            return sendError(BAD_REQUEST, conn);
        return;
    }

    if (endPos > MAX_HEADER_SIZE)
        return sendError(BAD_REQUEST, conn);

    std::string raw(conn.recvBuffer.data(), endPos);

    std::cout << raw;
    parser_.parse(raw);
    conn.req = parser_.getRequest();

    log(INFO, conn.req.version + " " + conn.req.method + " " + conn.req.uri);

    if (!validateRequest(conn))
        return;

    if (conn.req.method == "GET" || conn.req.method == "DELETE")
        return handleSimpleRequest(conn, endPos);

    if (conn.req.method == "POST")
        return startBodyReading(conn, endPos);

    return sendError(METHOD_NOT_ALLOWED, conn);
}

bool Server::validateRequest(Connection& conn) {
    std::string path = conn.req.path;

    LocationConfig location = resolveLocation(path);

    int status = checkRequest(conn.req, location);
    if (status != OK) {
        sendError(status, conn);
        return false;
    }

    if (location.redirectCode != 0) {
        sendRedirect(conn, location);
        return false;
    }

    status = resolvePath(path, location);

    if (status == DIRECTORY_NO_INDEX && location.autoindex) {
        generateAutoindex(conn, location);
        return false;
    }

    if (status != OK) {
        conn.state = PROCESSING;
        sendError(status, conn);
        return false;
    }

    return true;
}

void Server::handleSimpleRequest(Connection& conn, size_t endPos) {
    conn.state = PROCESSING;

    conn.recvBuffer.consume(endPos);

    handleRequest(conn);
}

void Server::startBodyReading(Connection& conn, size_t endPos) {
    StringMap::iterator it = conn.req.headers.find("Transfer-Encoding");

    if (it != conn.req.headers.end()) {
        if (it->second != "chunked")
            return sendError(BAD_REQUEST, conn);

        conn.state = READING_CHUNK_SIZE;
        conn.req.body.append(conn.recvBuffer.data(), endPos);
        conn.recvBuffer.consume(endPos);

        return;
    }
    
    it = conn.req.headers.find("Content-Length");

    if (it == conn.req.headers.end())
        return sendError(BAD_REQUEST, conn);

    conn.req.body.append(conn.recvBuffer.data(), endPos);
    conn.recvBuffer.consume(endPos);

    conn.remainingBody = std::strtoul(it->second.c_str(), NULL, 10);

    LocationConfig location = resolveLocation(conn.req.path);

    if (location.hasClientMaxBodySize &&
        conn.remainingBody > location.clientMaxBodySize)
    {
        return sendError(PAYLOAD_TOO_LARGE, conn);
    }

    conn.state = READING_BODY;
}

void Server::processChunkedBody(Connection& conn) {
    while (conn.recvBuffer.size() > 0) {
        std::cout << "WTF " << conn.state << std::endl;
        std::cout << conn.recvBuffer.size() << std::endl;
        if (conn.state == READING_CHUNK_SIZE) {
            // look for \r\n
            const char* data = conn.recvBuffer.data();
            size_t len = conn.recvBuffer.size();
            size_t i;
            for (i = 0; i + 1 < len; ++i) {
                if (data[i] == '\r' && data[i+1] == '\n') {
                    break;
                }
            }

            if (i + 1 >= len) // haven't received full line yet
                return;

            // parse hex chunk size
            std::string line(data, i);
            conn.currentChunkSize = strtoul(line.c_str(), NULL, 16);

            conn.recvBuffer.consume(i + 2); // remove chunk size line + \r\n

            if (conn.currentChunkSize == 0) {
                conn.state = PROCESSING;
                finishBody(conn);
                return;
            }

            conn.state = READING_CHUNK_DATA;
        }

        if (conn.state == READING_CHUNK_DATA) {
            size_t available = conn.recvBuffer.size();
            size_t toConsume = std::min(available, conn.currentChunkSize);

            conn.req.body.append(conn.recvBuffer.data(), toConsume);
            conn.recvBuffer.consume(toConsume);
            conn.currentChunkSize -= toConsume;

            if (conn.currentChunkSize == 0)
                conn.state = READING_CHUNK_CRLF;
            else
                return; // wait for more data
        }

        if (conn.state == READING_CHUNK_CRLF) {
            // expect \r\n after chunk data
            if (conn.recvBuffer.size() < 2)
                return; // wait for more

            conn.recvBuffer.consume(2);
            conn.state = READING_CHUNK_SIZE; // next chunk
        }
    }
}

void Server::processBody(Connection& conn) {
    if (conn.state == READING_CHUNKS ||
        conn.state == READING_CHUNK_SIZE ||
        conn.state == READING_CHUNK_DATA ||
        conn.state == READING_CHUNK_CRLF)
    {
        std::cout << "CHUNKS " << conn.req.body.size() << std::endl;
        processChunkedBody(conn);
        return;
    }

    // normal Content-Length body
    size_t available = conn.recvBuffer.size();
    if (available == 0)
        return;

    size_t toConsume = std::min(available, conn.remainingBody);
    conn.req.body.append(conn.recvBuffer.data(), toConsume);
    conn.recvBuffer.consume(toConsume);
    conn.remainingBody -= toConsume;

    if (conn.remainingBody == 0)
        finishBody(conn);
}

void Server::finishBody(Connection& conn) {
    conn.state = PROCESSING;

    std::cout << "Body finished\n";
    std::cout << "Size: " << conn.req.body.size() << std::endl;
    parser_.parse(conn.req.body);
    conn.req = parser_.getRequest();
    std::cout << "Parsed\n";

    handleRequest(conn);
}
//====================================================


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
		Connection& conn = connections_[clientFd];
		conn.fd = clientFd;
        conn.sendingFile = false;
        conn.remainingBody = 0;
        conn.state = READING_HEADERS;
        conn.lastActivityTime = std::time(NULL);
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

	if (listen(serverFd_, 10) < 0) {
		oss << "listen() failed on port " << config_.port;
		throw std::runtime_error(oss.str());
	}
	
	fcntl(serverFd_, F_SETFL, O_NONBLOCK);

	oss << "Server started on port ";
	oss << config_.port;
	log(INFO, oss.str()); 
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
