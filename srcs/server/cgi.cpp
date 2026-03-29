#include <string.h>
#include <sstream>
#include <unistd.h>
#include <fcntl.h> 
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
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
        std::string contentLength = "CONTENT_LENGTH=" + toString(req.bodySize);
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
    for (int j = 0; j < i; j++) {
        std::cout << env[j] << std::endl;
    }
    return env;
}

void Server::runCGI(const char* cgiPath, Connection& conn) {
    char tmpl[] = "/tmp/webserver_cgiXXXXXX";
    int tmpFd = mkstemp(tmpl);
    if (tmpFd == -1)
        return log(ERROR, "Failed to create tmp file");
    close(tmpFd);
    std::cout << "Running cgi\n";
    std::cout << "Out: " << tmpl << std::endl;

    pid_t pid = fork();
    if (pid < 0)
        return log(ERROR, "Fork failed");

    if (pid == 0) {
        int outFd = open(tmpl, O_WRONLY | O_TRUNC);
        if (outFd == -1)
            _exit(1);

        dup2(outFd, STDOUT_FILENO);
        dup2(outFd, STDERR_FILENO);
        close(outFd);

        if (conn.req.method == "POST") {
            std::cout << "Opening " << conn.req.tempFilePath << std::endl;
            int fd = open(conn.req.tempFilePath.c_str(), O_RDONLY);
            std::cout << "Fd = " << fd << std::endl;
            if (fd == -1)
                _exit(1);

            dup2(fd, STDIN_FILENO);
            std::cout << "Dubed\n";
            close(fd);
        }

        char* argv[] = { const_cast<char*>(cgiPath),
                         const_cast<char*>(conn.req.path.c_str()),
                         NULL};
        char** env = createEnvironment(conn.req);
        std::cout << "Running " << cgiPath << std::endl;
        execve(cgiPath, argv, env);
        _exit(1);
    }

    cgiProcesses_.push_back(CGIProcess(pid, tmpl, &conn));
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

void Server::handleCGIRead(Connection& conn, const std::string& tmpFilePath) {
    std::string raw;
    std::string headers;
    char buf[BUFFER_SIZE];

    std::cout << "Opening " << tmpFilePath << std::endl;
    int fd = open(tmpFilePath.c_str(), O_RDONLY);
    if (fd < 0) {
        log(ERROR, "Can't open file");
        return sendError(SERVER_ERROR, conn);
    }

    ssize_t totalSize = getFileSize(tmpFilePath);
    if (totalSize < 0) {
        log(ERROR, "Can't get file size");
        close(fd);
        return sendError(SERVER_ERROR, conn);
    }

    size_t pos = std::string::npos;

    while (true) {
        ssize_t n = read(fd, buf, BUFFER_SIZE);
        std::cout << n << std::endl;
        if (n > 0) {
            raw.append(buf, n);

            pos = raw.find("\r\n\r\n");
            if (pos != std::string::npos)
                break;
        } else if (n == 0) {
            break;
        } else {
            close(fd);
            return sendError(SERVER_ERROR, conn);
        }
    }

    if (pos == std::string::npos) {
        conn.res.contentLength = "0";
        conn.sendBuffer.append(generateHeader(conn.res));
        modifyToWrite(conn.fd);
        return;
    }

    headers = raw.substr(0, pos);
    parseCGIHeaders(conn.res, headers);

    size_t bodyStart = pos + 4;
    std::string remainder = raw.substr(bodyStart);

    conn.res.contentLength = toString(totalSize - bodyStart);

    std::string responseHeader = generateHeader(conn.res);
    conn.sendBuffer.append(responseHeader);
    std::cout << "Body: " << remainder << std::endl;
    conn.sendBuffer.append(remainder);

    conn.fileBuffer = fd;
    conn.state = SENDING_FILE;

    modifyToWrite(conn.fd);
}
