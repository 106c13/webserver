#include <string.h>
#include <sstream>
#include <unistd.h>
#include "defines.h"
#include "webserv.h"

char** Server::createEnvironment(const Request& req) {
    char** env = new char*[11];
    int i = 0;

    std::string method = "REQUEST_METHOD=" + req.method;
    env[i++] = strdup(method.c_str());

	std::string redirectStatus = "REDIRECT_STATUS=200";
	env[i++] = strdup(redirectStatus.c_str());

    std::string script = "SCRIPT_FILENAME=" + req.path;
    env[i++] = strdup(script.c_str());

    std::string query = "QUERY_STRING=" + req.queryString;
    env[i++] = strdup(query.c_str());

    std::string protocol = "SERVER_PROTOCOL=HTTP/1.1";
    env[i++] = strdup(protocol.c_str());

    std::string gateway = "GATEWAY_INTERFACE=CGI/1.1";
    env[i++] = strdup(gateway.c_str());

    std::string serverName = "SERVER_NAME=127.0.0.1";
    env[i++] = strdup(serverName.c_str());

    std::string serverPort = "SERVER_PORT=" + toString(config_.port);
    env[i++] = strdup(serverPort.c_str());

    if (req.method == "POST") {
        std::string contentLength =
            "CONTENT_LENGTH=" + req.headers.at("Content-Length");
        env[i++] = strdup(contentLength.c_str());

        std::string contentType =
            "CONTENT_TYPE=" + req.headers.at("Content-Type");
        env[i++] = strdup(contentType.c_str());
    }

    env[i] = NULL;

    return env;
}

void Server::sendCGIOutput(Connection& conn, int cgiFd) {
    std::string cgiOut;
    char buf[8192];
    ssize_t n;

    while ((n = read(cgiFd, buf, sizeof(buf))) > 0)
        cgiOut.append(buf, n);

    close(cgiFd);

    Response& res = conn.res;

    size_t pos = cgiOut.find("\r\n\r\n");
    std::string cgiHeaders;
    std::string body;

    if (pos != std::string::npos)
    {
        cgiHeaders = cgiOut.substr(0, pos);
        body = cgiOut.substr(pos + 4);
    }
    else
    {
        body = cgiOut;
    }

    // Default values
    res.contentType = "text/html";
    res.status = OK;

    // Parse CGI headers (C++98 style)
    std::istringstream stream(cgiHeaders);
    std::string line;

    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (line.find("Status:") == 0)
        {
            res.status = atoi(line.substr(7).c_str());
        }
        else if (line.find("Location:") == 0)
        {
            res.location = line.substr(10);
        }
    }

    res.contentLength = toString(body.size());

    std::string header = generateHeader(res);
    std::cout << header;

    conn.sendBuffer.append(header);
    conn.sendBuffer.append(body);
    conn.closed = true;

    modifyToWrite(conn.fd);
}

int Server::runCGI(const char* cgiPath, Connection& conn) {
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

        close(stdinPipe[1]);
        close(stdoutPipe[0]);
        close(stdinPipe[0]);
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

    if (conn.req.method == "POST") {
        const std::string& body = conn.req.body;
        size_t size = body.size();
        size_t total = 0;

        while (total < size) {
            ssize_t n = write(stdinPipe[1],
                              body.data() + total,
                              size - total);

            if (n <= 0) {
                log(ERROR, "WRITE TO CGI STDIN FAILED");
                break;
            }

            total += n;
        }
    }

    close(stdinPipe[1]);

    return stdoutPipe[0];
}

std::string Server::findCGI(const std::string& fileName, const StringMap& cgiMap) {
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
