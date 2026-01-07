#include <string.h>
#include <fcntl.h>
#include "webserv.h"

static char**	createEviroment(const HttpRequest& request) {
	char **env = new char*[5]; 
	// parsing the request
	(void)request;
	// ==================== TEST =====================
	env[0] = strdup("REQUEST_METHOD=GET");
    env[1] = strdup("SCRIPT_FILENAME=/tmp/index.php");
	env[2] = strdup("REDIRECT_STATUS=200");
	env[3] = strdup("QUERY_STRING=name=Karlito&age=42");
    env[4] = NULL;
	// ===============================================
	return env;
}

int Server::runCGI(const char* path, const HttpRequest& request)
{
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
		dup2(pipefd[1], STDOUT_FILENO);
		close(pipefd[1]);
		env = createEviroment(request);
		char* argv[] = {
			strdup(path),
			strdup(request.getFile().c_str()),
            NULL
        };
		execve(path, argv, env);
		_exit(1);
	}
	close(pipefd[1]);
    return pipefd[0];
}

void	Server::handleRequest(HttpRequest& request) {
	// 1) Parse the resquest
	// 2) Get the file name or directory from request->http->path
	//    If path ends with / then add default file name like index.html to the path
	// 3) Read the file request->http->path
	request.setFile(config_.root + "/index.html");
	std::string path = request.getFile();
	std::string	response;
	int			fd;

	log(INFO, "<method> <path> <http_version> " + request.get());
	if (!fileExists(path)) {
		return sendError(NOT_FOUND, request);
	}
	if (!canReadFile(path)) {
		return sendError(FORBIDDEN, request);
	}
	// 1) When no CGI, just simple read file and send it
	fd = open(path.c_str(), O_RDONLY);
	request.sendAll(path, fd);
	close(fd);
	// response = readFile(path);
	// 2) If CGI is on, then prepare the enviroment
	// fd = runCGI("./php-cgi", request);
	// if (fd < 0)
	// 	return sendError(SERVER_ERROR, request);
	// request.sendAll(fd);
}

void	Server::acceptConnection() {
	int			request_fd;
	sockaddr_in	request_addr;
	socklen_t	request_len = sizeof(request_addr);

	request_fd = accept(server_fd_, (sockaddr *)&request_addr, &request_len);
	if (request_fd < 0) {
		log(ERROR, "accept() failed");
		return;
	}
	HttpRequest request(request_fd);
	handleRequest(request);
}
