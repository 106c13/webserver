#ifndef WEBSERV_H
#define WEBSERV_H

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <exception>

enum {
	INFO = 10,
	WARNING = 11,
	ERROR = 12,

	GET = 100,
	POST = 101,

	PAGE_NOT_FOUND = 1000,
	FORBIDDEN = 1001,
	SERVER_ERROR = 1002,
	BAD_REQUEST = 1003

};

struct ServerConfig {
	int			port;
	std::string	homeDir;
	ServerConfig() : port(8080), homeDir("/tmp") {}
};

class	Request {
	private:
		// I don't know
		int			request_fd_;
		int			method_;
		std::string	content_;
	public:
		Request(int	fd);
		~Request();

		const std::string&	get(); // I don't know what to write here
		int					sendAll(const std::string& response);
};

class	Server {
	private:
		// config
		ServerConfig	config_;

		// variables
		int			server_fd_;
		sockaddr_in	addr_;

	public:
		Server(); // Start server with default configurations
		Server(std::string& fileName); // Start server with configurations from file
		~Server();

		void	acceptConnection();
		void	handleRequest(Request&	request);

		void	sendError(int code, Request& request) const;
};

void		log(int type, const std::string& msg);
bool		fileExists(const std::string& path);
bool		canReadFile(const std::string& path);
std::string	readFile(const std::string& filename);

#endif
