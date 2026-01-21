static char** createEviroment(const HttpRequest& request) {
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
			strdup(request.getPath().c_str()),
            NULL
        };
		execve(path, argv, env);
		_exit(1);
	}
	close(pipefd[1]);
    return pipefd[0];
}
