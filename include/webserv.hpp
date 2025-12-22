#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <exception>

enum {
	INFO = 100,
	WARNING = 101,
	ERROR = 102,

	GET = 1000,
	POST = 1001
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
		void	pageNotFound(Request& request) const;
};

void	log(int type, const std::string& msg);
bool	fileExists(const std::string& path);

#endif
