#ifndef WEBSERV_H
#define WEBSERV_H

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <exception>
#include <dirent.h>
#include "ConfigParser.h"
#include "RequestParser.h"

#define COLOR_GREEN "\033[1;32m"
#define COLOR_RED   "\033[1;31m"
#define COLOR_RESET "\033[0m"

typedef std::vector<LocationConfig> LocationList;

enum Log {
	INFO = 10,
	WARNING = 11,
	ERROR = 12
};

enum Method {
	GET = 358774,
	POST = 101
};

enum Page {
	NOT_FOUND = 1000,
	FORBIDDEN = 1001,
	SERVER_ERROR = 1002,
	BAD_REQUEST = 1003
};

struct DirEntry {
	std::string name;
	bool		is_dir;
};

class	HttpRequest {
	private:
		// I don't know
		int			request_fd_;
		std::string content_;
		Request		body_;
	public:
		HttpRequest(int	fd);
		~HttpRequest();

		void				setRequest(const Request& request);
		void				setFile(const std::string& file);

		const std::string&	get(); // I don't know what to write here
		const std::string&	getPath() const;
		const std::string&	getMethod() const;
		const std::string&	getURI() const;

		int					sendAll(const std::string& response);
		int					sendAll(const std::string& path, int fd);
		int					sendAll(const int fd);
};

class	Server {
	private:
		// config
		ServerConfig	config_;
		RequestParser	parser_;

		// variables
		int			server_fd_;
		sockaddr_in	addr_;

		void			initSocket();
		int				runCGI(const char* path, const HttpRequest& request);
		void			handleRequest(HttpRequest&	request);
		int				resolvePath(std::string& path, LocationConfig& location);
		LocationConfig&	resolveLocation(std::string& fs_path);

	public:
		Server(const ServerConfig& config); // Start server with configurations from file
		~Server();

		void	acceptConnection();
		void	sendError(int code, HttpRequest& request) const;
		void	generate_autoindex(HttpRequest& request, LocationConfig& location);
};

void					log(int type, const std::string& msg);
void					log(const HttpRequest& request);
int						get_path_type(const char *path);
bool					fileExists(const std::string& path);
bool					canReadFile(const std::string& path);
ssize_t					getFileSize(const std::string& path);
std::string				readFile(const std::string& filename);
std::vector<DirEntry>	list_directory(const std::string& path);
#endif
