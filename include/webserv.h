#ifndef WEBSERV_H
#define WEBSERV_h

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <exception>

enum {
	INFO = 100,
	WARNING = 101,
	ERROR = 102
};

struct ServerConfig {
	int	port;

	ServerConfig() : port(8080) {}
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

		class	SocketError : public std::exception {
			public:
				const char*	what() const throw();
		};

};

#endif
