#include <unistd.h>  
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <ctime>
#include <iostream>
#include <exception>
#include <cerrno>
#include <cstring>
#include <sstream>
#include <cstdio>
#include <algorithm>
#include "webserv.h"
#include "ConfigParser.h"

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

static bool isMethodAllowed(const std::string& method, const std::vector<std::string>& methods) {
    for (std::vector<std::string>::const_iterator it = methods.begin();
         it != methods.end();
         ++it)
    {
        if (*it == method)
            return true;
    }
    return false;
}

static int checkMethod(const Request& req, const LocationConfig& location) {
    const std::string& method = req.method;

    if (location.methods.empty()) {
        if (method == "GET" ||
            method == "POST" ||
            method == "DELETE")
            return OK;

        return BAD_REQUEST;
    }

    if (isMethodAllowed(req.method, location.methods))
        return OK;

    return METHOD_NOT_ALLOWED;
}

static int deleteResource(const std::string& path) {
    if (access(path.c_str(), W_OK) != 0)
        return FORBIDDEN;

    if (unlink(path.c_str()) != 0)
        return SERVER_ERROR;


    return NO_CONTENT;
}

ServerConfig* ServerManager::findServerConfig(int port, const std::string& host) {
    std::vector<ServerConfig>& configs = ports_[port];
    
    // 1. Try to match server_name
    for (size_t i = 0; i < configs.size(); ++i) {
        for (size_t j = 0; j < configs[i].serverNames.size(); ++j) {
            if (configs[i].serverNames[j] == host)
                return &configs[i];
        }
    }
    
    // 2. Default to first config for this port
    return &configs[0];
}

void ServerManager::processHeaders(Connection& conn) {
    size_t endPos;

    if (!requestComplete(conn.buffer, endPos)) {
        if (conn.buffer.size() > MAX_HEADER_SIZE)
            return sendError(BAD_REQUEST, conn);
        return;
    }

    if (endPos > MAX_HEADER_SIZE)
        return sendError(BAD_REQUEST, conn);

    std::string headerStr(conn.buffer.data(), endPos);
    conn.buffer.consume(endPos);

    RequestParser parser;
    parser.parse(headerStr);
    conn.req = parser.getRequest();

    // Dispatch to correct server config based on Host header
    std::string host = conn.req.headers["Host"];
    // Remove port if present in Host header (e.g. localhost:8080 -> localhost)
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos)
        host = host.substr(0, colonPos);
    
    conn.config = findServerConfig(conn.port, host);
    conn.location = resolveLocation(conn.req.path, conn.config->locations);

    log(INFO, conn.req.version + " " + conn.req.method + " " + conn.req.uri + " (Port: " + toString(conn.port) + ")");

	int fileStatus = resolvePath(conn.req.path, conn.location);
    bool cM = true;

    std::string cgiPath = findCGI(conn.req.path, conn.config->cgi.extensions);
	std::cout << "M cgi: " << cgiPath << std::endl;
    if (!cgiPath.empty()) {
        if (isMethodAllowed(conn.req.method, conn.config->cgi.methods)) {
            cM = false;
            conn.req.cgiPath = cgiPath;
        }
    } else {
        cgiPath = findCGI(conn.req.path, conn.location.cgi);
		std::cout << "cgi: " << cgiPath << std::endl;
        if (!cgiPath.empty()) {
			cM = false;
            conn.req.cgiPath = cgiPath;
		}
    }

    int status;

    if (cM) {
        status = checkMethod(conn.req, conn.location);
        if (status != OK)
            return sendError(status, conn);
    }


    if (fileStatus == DIRECTORY_NO_INDEX && conn.location.autoindex)
		return generateAutoindex(conn);

	std::cout << fileStatus << std::endl;

    if (fileStatus != OK && cgiPath.empty() && !conn.location.root.empty())
        return sendError(fileStatus, conn);
    
    if (conn.location.redirectCode != 0)
        return sendRedirect(conn);

    if (conn.req.method == "GET")
        return handleGet(conn);

    if (conn.req.method == "DELETE")
        return sendError(deleteResource(conn.req.path), conn);

    return startBodyReading(conn);
}

void ServerManager::acceptConnection(int serverFd) {
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int port = listeningSockets_[serverFd];

    while (true) {
        int clientFd = accept(serverFd, (sockaddr*)&addr, &len);
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
        conn.port = port;
        conn.state = READING_HEADER;
        conn.lastActivityTime = std::time(NULL);

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));

        std::ostringstream oss;
        oss << "New connection on port " << port << " from " << ip;
        log(INFO, oss.str());
    }
}

void ServerManager::initSockets() {
    for (std::map<int, std::vector<ServerConfig> >::iterator it = ports_.begin(); it != ports_.end(); ++it) {
        int port = it->first;
        int serverFd = socket(AF_INET, SOCK_STREAM, 0);

        if (serverFd < 0) {
            throw std::runtime_error("socket() failed");
        }

        int opt = 1;
        if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            throw std::runtime_error("setsockopt() failed");
        }

        sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            std::ostringstream oss;
            oss << "bind() failed on port " << port;
            throw std::runtime_error(oss.str());
        }

        if (listen(serverFd, 128) < 0) {
            std::ostringstream oss;
            oss << "listen() failed on port " << port;
            throw std::runtime_error(oss.str());
        }
        
        fcntl(serverFd, F_SETFL, O_NONBLOCK);

        listeningSockets_[serverFd] = port;

#ifdef __linux__
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = serverFd;
        epoll_ctl(epollFd_, EPOLL_CTL_ADD, serverFd, &ev);
#elif __APPLE__
        struct kevent ev;
        EV_SET(&ev, serverFd, EVFILT_READ, EV_ADD, 0, 0, NULL);
        kevent(epollFd_, &ev, 1, NULL, 0, NULL);
#endif
        std::ostringstream oss;
        oss << "Listening on port " << port;
        log(INFO, oss.str());
    }
}

void ServerManager::checkCGIProcesses() {
    std::vector<CGIProcess>::iterator it = cgiProcesses_.begin();

    while (it != cgiProcesses_.end()) {
        int status;
        pid_t result = waitpid(it->pid, &status, WNOHANG);

        if (result > 0) {
            if (it->conn) {
                handleCGIRead(*(it->conn), it->tmpFilePath);
            }

            unlink(it->tmpFilePath.c_str());
            it = cgiProcesses_.erase(it);
        } else {
            ++it;
        }
    }
}

void ServerManager::run() {
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
        if (evCount < 0)
            continue;

        for (int i = 0; i < evCount; i++) {
            int fd;
#ifdef __linux__
            fd = events[i].data.fd;
#elif __APPLE__
            fd = (int)events[i].ident;
#endif

            if (listeningSockets_.count(fd)) {
                acceptConnection(fd);
            } else {
                try {
                    handleClient(events[i]);
                } catch (std::exception& e) {
                    log(ERROR, e.what());
                }
            }
        }
    }
}

ServerManager::ServerManager(const Config& config) : fullConfig_(config) {
#ifdef __linux__
	epollFd_ = epoll_create(1024);
#elif __APPLE__
	epollFd_ = kqueue();
#endif
    if (epollFd_ < 0)
        throw std::runtime_error("Failed to create multiplexing instance");

    // Group configs by port
    for (size_t i = 0; i < config.servers.size(); ++i) {
        ports_[config.servers[i].port].push_back(config.servers[i]);
    }
	
	initSockets();
}

ServerManager::~ServerManager() {
    for (std::map<int, int>::iterator it = listeningSockets_.begin(); it != listeningSockets_.end(); ++it) {
        close(it->first);
    }
    if (epollFd_ >= 0)
        close(epollFd_);
}
