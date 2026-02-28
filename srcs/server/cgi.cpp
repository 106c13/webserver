#include <string.h>
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

    std::string body;

    size_t pos = cgiOut.find("\r\n\r\n");
    if (pos != std::string::npos) {
        body = cgiOut.substr(pos + 4);
    } else {
        body = cgiOut;
    }

    Response& res = conn.res;
    res.status = OK;
    res.contentLength = toString(body.size());
    res.contentType = "text/html";

    std::string header = generateHeader(res);
    std::cout << header << std::endl;

    conn.sendBuffer.append(header);
    conn.sendBuffer.append(body);

    modifyToWrite(conn.fd);
}

int Server::runCGI(const char* cgiPath, Connection& conn) {
	int		pipefd[2];
	pid_t	pid;
	char**	env;

	if (pipe(pipefd) < 0) {
		log(ERROR, "PIPE ERROR");
		return -1;
	}

	pid = fork();
	if (pid < 0) {
		log(ERROR, "FORK ERROR");
		return -1;
	} else if (pid == 0) {
		close(pipefd[0]);

		env = createEnvironment(conn.req);
		dup2(pipefd[1], STDOUT_FILENO);
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);
		char* argv[] = {
			strdup(cgiPath),
			strdup(conn.req.path.c_str()),
            NULL
        };
		execve(cgiPath, argv, env);
		_exit(1);
	}

	close(pipefd[1]);
    return pipefd[0];
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
