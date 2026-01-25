#include <string.h>
#include "webserv.h"

static char** createEviroment(const HttpRequest& request)
{
    char** env = new char*[5];

    std::string method = "REQUEST_METHOD=" + request.getMethod();
    env[0] = strdup(method.c_str());
    std::string script = "SCRIPT_FILENAME=" + request.getPath();
    env[1] = strdup(script.c_str());
    env[2] = strdup("REDIRECT_STATUS=200");
    std::string query = "QUERY_STRING=";
    env[3] = strdup(query.c_str());
    env[4] = NULL;
    return env;
}

int Server::runCGI(const char* path, const char* cgiPath, const HttpRequest& request){
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
		dup2(pipefd[1], STDERR_FILENO);
		close(pipefd[1]);
		env = createEviroment(request);
		char* argv[] = {
			strdup(cgiPath),
			strdup(path),
            NULL
        };
		execve(cgiPath, argv, env);
		_exit(1);
	}
	close(pipefd[1]);
    return pipefd[0];
}

std::string Server::findCGI(const std::string& fileName, const std::map<std::string, std::string>& cgiMap) {
	std::string extension;

	for (int i = fileName.size() - 1; i >= 0; i--) {
		if (fileName[i] == '.') {
			extension = fileName.substr(i, fileName.size() - i);
			break;
		}
	}
	std::cout << extension << std::endl;
	if (extension.empty())
        return "";
	std::map<std::string, std::string>::const_iterator it = cgiMap.find(extension);
	if (it == cgiMap.end())
		return "";
	return it->second;
}
