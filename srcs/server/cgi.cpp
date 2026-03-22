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
        std::map<std::string,std::string>::const_iterator itCL = req.headers.find("Content-Length");
        if (itCL != req.headers.end()) {
            std::string contentLength = "CONTENT_LENGTH=" + itCL->second;
            env[i++] = strdup(contentLength.c_str());
        }

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
        std::cout << env[i - 1] << std::endl;
    }

    env[i] = NULL;
    return env;
}

void Server::runCGI(const char* cgiPath, Connection& conn) {
    int stdinPipe[2];
    int stdoutPipe[2];

    if (pipe(stdinPipe) < 0 || pipe(stdoutPipe) < 0) {
        log(ERROR, "PIPE ERROR");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        log(ERROR, "FORK ERROR");
        return -1;
    }

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

    CGIProcess proc(pid, DEFAULT, stdinPipe[1], stdoutPipe[0], conn);
    cgiProcesses_.push_back(proc);
    
    addEvent(conn.cgiStdin, false, true);
    addEvent(conn.cgiStdout, true, false);
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
    CGI* cgi = conn.cgi;

    size_t remaining = req.bodySize - req.bodySent;

    if (remaining > BUFF_SIZE)
        remaining = BUFF_SIZE;

    ssize_t n = write(cgi->Stdin,
                     req.body.c_str() + req.bodySent,
                     remaining);
    
    if (n > 0) {
        req.bodySent += n;

        if (req.bodySent >= req.bodySize)
            return closeCgiFd(cgi->cgiStdin);
    }

    if (n == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;

        log(ERROR, "CGI write failed");
            return closeCgiFd(cgi->cgiStdin);
    }
}

void Server::handleCGIRead(Connection& conn) {
    char buf[BUFFER_SIZE];

    while (true) {
        ssize_t n = read(conn.cgiStdout, buf, BUFFER_SIZE);
        //std::cout << n << std::endl;
        if (n > 0) {
            // Append everything to temporary CGI output buffer
            conn.cgiRawOutput.append(buf, n);

            return;
        }

        if (n == 0) {
            //std::cout << "In n = 0\n";
            // EOF → CGI finished
            closeCgiFd(conn.cgiStdout);

            int status;
            waitpid(conn.cgiPid, &status, WNOHANG);

            // Now parse CGI headers and prepare sendBuffer
            size_t headerEnd = conn.cgiRawOutput.find("\r\n\r\n");
            std::string cgiHeaders;
            std::string body;

            if (headerEnd != std::string::npos) {
                cgiHeaders = conn.cgiRawOutput.substr(0, headerEnd);
                body = conn.cgiRawOutput.substr(headerEnd + 4);
            } else {
                body = conn.cgiRawOutput; // no headers
            }

            Response& res = conn.res;
            res.status = OK;           // default
            res.contentType = "text/html";

            std::istringstream stream(cgiHeaders);
            std::string line;
            while (std::getline(stream, line)) {
                if (!line.empty() && line[line.size()-1] == '\r')
                    line.resize(line.size()-1); // old-style pop_back

                if (line.find("Status:") == 0)
                    res.status = atoi(line.substr(7).c_str());
                else if (line.find("Content-Type:") == 0)
                    res.contentType = line.substr(13);
                else if (line.find("Location:") == 0)
                    res.location = line.substr(10);
            }

            // Set Content-Length now, because we have full body
            res.contentLength = toString(body.size());

            // Generate HTTP header
            std::string header = generateHeader(res);
            std::cout << header;
            // Fill sendBuffer with header + body
            conn.sendBuffer.append(header);
            conn.sendBuffer.append(body);

            // Switch to sending response
            conn.state = SENDING_RESPONSE;
            modifyToWrite(conn.fd);

            conn.cgiRawOutput.clear();
            return;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No more data ready for now, return and wait for epoll
            return;
        }

        // Any other read error
        log(ERROR, "CGI stdout read failed");
        closeCgiFd(conn.cgiStdout);
        sendError(SERVER_ERROR, conn);
        return;
    }
}
