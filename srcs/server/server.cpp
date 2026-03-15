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
        conn.state == READING_CHUNK_CRLF ||
        conn.state == CGI_WRITING_BODY ||
        conn.state == CGI_BUFFERING_TO_FILE)
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
    conn.headerSize = endPos;
    std::cout << "header Size: " << conn.headerSize << std::endl;

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

    conn.location = resolveLocation(path);
    LocationConfig& location = conn.location; 

    std::cout << "Location: " << location.path << " " << location.methods[0] << std::endl;

    int status = checkRequest(conn.req, location);
    if (status != OK) {
        sendError(status, conn);
        return false;
    }

    if (location.redirectCode != 0) {
        sendRedirect(conn, location);
        return false;
    }


    std::string cgiPath = findCGI(path, location.cgi);
    if (!cgiPath.empty()) { 
        return true;
    }

    // ============= Check size =================
    StringMap::iterator it = conn.req.headers.find("Transfer-Encoding");

    if (it != conn.req.headers.end()) {
        if (it->second != "chunked") {
            sendError(BAD_REQUEST, conn);
            return false;
        }
    } else if (conn.req.method == "POST"){
        it = conn.req.headers.find("Content-Length");

        if (it == conn.req.headers.end()) {
            sendError(BAD_REQUEST, conn);
            return false;
        }

        size_t length = std::strtoul(it->second.c_str(), NULL, 10);
        std::cout << "TEST length = " << length << std::endl;

        if (location.hasClientMaxBodySize &&
            length > location.clientMaxBodySize)
        {
            sendError(PAYLOAD_TOO_LARGE, conn);
            return false;
        }
    }
    // ==============================================

    if (location.root.empty()) {
        return true;
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

    conn.remainingBody = std::strtoul(it->second.c_str(), NULL, 10);

    LocationConfig& location = conn.location;

    if (location.hasClientMaxBodySize &&
        conn.remainingBody > location.clientMaxBodySize)
    {
        conn.recvBuffer.consume(endPos);
        return sendError(PAYLOAD_TOO_LARGE, conn);
    }

    // For CGI requests with large bodies, buffer to temp file
    // to avoid holding everything in memory.
    std::string path = conn.req.path;
    resolvePath(path, location);
    conn.req.path = path;
    conn.res.path = path;
    std::string cgiPath = findCGI(path, location.cgi);
    if (!cgiPath.empty()) {
        conn.recvBuffer.consume(endPos);
        conn.cgiExecPath = cgiPath;

        if (conn.remainingBody > 1024 * 1024) {
            // Large body: write to temp file, start CGI after
            char tmpl[] = "/tmp/webserv_cgi_XXXXXX";
            int tmpFd = mkstemp(tmpl);
            if (tmpFd < 0)
                return sendError(SERVER_ERROR, conn);
            conn.cgiBodyTmpFd = tmpFd;
            conn.cgiBodyTmpPath = tmpl;
            conn.state = CGI_BUFFERING_TO_FILE;
            return;
        }

        // Small body: stream directly to CGI
        std::cout << "Running cgi (streaming)\n";
        conn.cgiWritePos = 0;
        runCGI(cgiPath.c_str(), conn);
        return;
    }

    // Non-CGI: keep headers in body for re-parsing in finishBody
    conn.req.body.append(conn.recvBuffer.data(), endPos);
    conn.recvBuffer.consume(endPos);
    conn.state = READING_BODY;
}

void Server::processChunkedBody(Connection& conn) {
    while (conn.recvBuffer.size() > 0) {
        //std::cout << conn.recvBuffer.size() << std::endl;
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
    LocationConfig& location = conn.location;

    if (conn.state == READING_CHUNKS ||
        conn.state == READING_CHUNK_SIZE ||
        conn.state == READING_CHUNK_DATA ||
        conn.state == READING_CHUNK_CRLF)
    {
        if (location.clientMaxBodySize > 0 &&
            conn.req.body.size() - conn.headerSize > location.clientMaxBodySize) {
            sendError(PAYLOAD_TOO_LARGE, conn);
            return;
        }

        processChunkedBody(conn);
        return;
    }

    // CGI large body: write incoming data to temp file
    if (conn.state == CGI_BUFFERING_TO_FILE) {
        size_t available = conn.recvBuffer.size();
        if (available == 0)
            return;

        size_t toWrite = std::min(available, conn.remainingBody);
        ssize_t n = write(conn.cgiBodyTmpFd, conn.recvBuffer.data(), toWrite);

        if (n > 0) {
            conn.recvBuffer.consume(n);
            conn.remainingBody -= n;
        } else if (n < 0) {
            close(conn.cgiBodyTmpFd);
            unlink(conn.cgiBodyTmpPath.c_str());
            conn.cgiBodyTmpFd = -1;
            sendError(SERVER_ERROR, conn);
            return;
        }

        if (conn.remainingBody == 0) {
            // Body fully written to file — rewind for reading
            lseek(conn.cgiBodyTmpFd, 0, SEEK_SET);

            if (cgiProcesses_.size() >= 5) {
                // Queue until a CGI slot opens
                cgiQueue_.push_back(conn.fd);
                return;
            }
            std::cout << "CGI body buffered to file, starting cgi\n";
            conn.cgiWritePos = 0;
            runCGI(conn.cgiExecPath.c_str(), conn);
        }
        return;
    }

    // CGI streaming: body piped by handleCGIWrite (epoll-driven).
    // Here we only drain if cgiStdin was already closed.
    if (conn.state == CGI_WRITING_BODY) {
        if (conn.cgiStdin < 0) {
            size_t available = conn.recvBuffer.size();
            size_t toDrain = std::min(available, conn.remainingBody);
            conn.recvBuffer.consume(toDrain);
            conn.remainingBody -= toDrain;
            if (conn.remainingBody == 0)
                conn.state = CGI_READING_OUTPUT;
        }
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
        conn.cgiHeaderSent = false;
        conn.cgiBodyTmpFd = -1;
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

void Server::startQueuedCGIs() {
    while (!cgiQueue_.empty() && cgiProcesses_.size() < 5) {
        int fd = cgiQueue_.front();
        cgiQueue_.erase(cgiQueue_.begin());

        std::map<int, Connection>::iterator it = connections_.find(fd);
        if (it == connections_.end())
            continue;

        Connection& conn = it->second;
        if (conn.state != CGI_BUFFERING_TO_FILE)
            continue;

        std::cout << "Starting queued cgi from file\n";
        conn.cgiWritePos = 0;
        runCGI(conn.cgiExecPath.c_str(), conn);
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
