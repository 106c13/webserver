#include <string.h>
#include <sstream>
#include <unistd.h>
#include <fcntl.h> 
#include <errno.h>
#include <sys/wait.h>
#include "defines.h"
#include "webserv.h"

char** Server::createEnvironment(const Request& req) {
    char** env = new char*[64]; 
    int i = 0;

    std::string method = "REQUEST_METHOD=" + req.method;
    env[i++] = strdup(method.c_str());

    env[i++] = strdup("REDIRECT_STATUS=200");

    std::string script = "SCRIPT_FILENAME=" + req.path;
    env[i++] = strdup(script.c_str());

    std::string pathInfo = "PATH_INFO=" + req.path;
    env[i++] = strdup(pathInfo.c_str());

    std::string query = "QUERY_STRING=" + req.queryString;
    env[i++] = strdup(query.c_str());

    env[i++] = strdup("SERVER_PROTOCOL=HTTP/1.1");
    env[i++] = strdup("GATEWAY_INTERFACE=CGI/1.1");
    env[i++] = strdup("SERVER_NAME=127.0.0.1");

    std::string serverPort = "SERVER_PORT=" + toString(config_.port);
    env[i++] = strdup(serverPort.c_str());

    if (req.method == "POST") {
        std::string contentLength = "CONTENT_LENGTH=" + req.bodySize;
        env[i++] = strdup(contentLength.c_str());

        std::map<std::string,std::string>::const_iterator itType = req.headers.find("Content-Type");
        if (itType != req.headers.end()) {
            std::string contentType = "CONTENT_TYPE=" + itType->second;
            env[i++] = strdup(contentType.c_str());
        }
    }

    for (std::map<std::string,std::string>::const_iterator it = req.headers.begin();
         it != req.headers.end(); ++it)
    {
        std::string name = it->first;

        std::string lname;
        for (size_t k = 0; k < name.size(); ++k)
            lname += tolower(name[k]);

        if (lname == "content-type" ||
            lname == "content-length" ||
            lname == "transfer-encoding" ||
            lname == "connection" ||
            lname == "host")
            continue;

        std::string envVar = "HTTP_";
        for (size_t k = 0; k < name.size(); ++k) {
            char c = name[k];
            envVar += (c == '-') ? '_' : toupper(c);
        }
        envVar += "=" + it->second;

        env[i++] = strdup(envVar.c_str());
    }

    env[i] = NULL;
    return env;
}

void Server::runCGI(const char* cgiPath, Connection& conn) {
    int stdinPipe[2];
    int stdoutPipe[2];

    if (pipe(stdinPipe) < 0 || pipe(stdoutPipe) < 0)
        return log(ERROR, "PIPE ERROR");

    pid_t pid = fork();

    if (pid < 0)
        return log(ERROR, "FORK ERROR");

    if (pid == 0) {
        dup2(stdinPipe[0], STDIN_FILENO);
        dup2(stdoutPipe[1], STDOUT_FILENO);
        dup2(stdoutPipe[1], STDERR_FILENO);

        close(stdinPipe[0]);
        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdoutPipe[1]);

        char** env = createEnvironment(conn.req);
        char* argv[] = {
            const_cast<char*>(cgiPath),
            const_cast<char*>(conn.req.path.c_str()),
            NULL
        };

        execve(cgiPath, argv, env);
        _exit(1);
    }

    close(stdinPipe[0]);
    close(stdoutPipe[1]);

    fcntl(stdinPipe[1], F_SETFL, O_NONBLOCK);
    fcntl(stdoutPipe[0], F_SETFL, O_NONBLOCK);

    cgiFdMap_[stdinPipe[1]] = &conn;
    cgiFdMap_[stdoutPipe[0]] = &conn;

    CGIProcess proc(pid, 0, stdinPipe[1], stdoutPipe[0], conn);
    cgiProcesses_.push_back(proc);
    
    addEvent(proc.Stdin, false, true);
    addEvent(proc.Stdout, true, false);
    conn.cgi = &proc;
}

std::string findCGI(const std::string& fileName, const StringMap& cgiMap) {
	std::string extension;

	for (int i = fileName.size() - 1; i >= 0; i--) {
		if (fileName[i] == '.') {
			extension = fileName.substr(i, fileName.size() - i);
			break;
		}
	}

	if (extension.empty())
        return "";

	std::map<std::string, std::string>::const_iterator it = cgiMap.find(extension);

	if (it == cgiMap.end()) {
		return "";
    }

	return it->second;
}

void Server::closeCgiFd(int& fd) {
    if (fd == -1)
        return;
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
    cgiFdMap_.erase(fd);
    fd = -1;
}

void Server::handleCGIWrite(Connection& conn) {
    Request& req = conn.req;
    CGIProcess* cgi = conn.cgi;

    size_t remaining = req.bodySize - req.bodySent;

    if (remaining > BUFFER_SIZE)
        remaining = BUFFER_SIZE;

    ssize_t n = write(cgi->Stdin,
                     req.body.c_str() + req.bodySent,
                     remaining);
    
    if (n > 0) {
        req.bodySent += n;

        if (req.bodySent >= req.bodySize)
            return closeCgiFd(cgi->Stdin);
    }

    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;

        log(ERROR, "CGI write failed");
            return closeCgiFd(cgi->Stdin);
    }
}

static void parseCGIHeaders(Response& res, const std::string& headers) {
    res.status = OK;
    res.contentType = "text/html";

    std::istringstream stream(headers);
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.resize(line.size() - 1);

        if (line.find("Status:") == 0)
            res.status = atoi(line.substr(7).c_str());
        else if (line.find("Content-Type:") == 0)
            res.contentType = line.substr(13);
        else if (line.find("Location:") == 0)
            res.location = line.substr(10);
    }
}

static void buildResponseFromCGI(Connection& conn) {
    std::string& raw = conn.cgi->rawOutput;

    size_t pos = raw.find("\r\n\r\n");

    std::string headers;
    if (pos != std::string::npos) {
        headers = raw.substr(0, pos);
        raw.erase(0, pos + 4);
    }

    Response& res = conn.res;
    res.status = OK;
    res.contentType = "text/html";

    parseCGIHeaders(res, headers);

    res.contentLength = toString(raw.size());
}

void Server::handleCGIRead(Connection& conn) {
    char buf[BUFFER_SIZE];
    ssize_t n = read(conn.cgi->Stdout, buf, BUFFER_SIZE);

    if (n > 0) {
        conn.cgi->rawOutput.append(buf, n);
        return;
    } else if (n == 0) {
        closeCgiFd(conn.cgi->Stdout);
        buildResponseFromCGI(conn);

        std::string header = generateHeader(conn.res);
        conn.sendBuffer.append(header);
        conn.sendBuffer.append(conn.cgi->rawOutput);
        conn.cgi->rawOutput.clear();
        modifyToWrite(conn.fd);
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        return sendError(SERVER_ERROR, conn);
    }
}
