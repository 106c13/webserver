#ifndef WEBSERV_H
#define WEBSERV_h

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>

enum {
	INFO = 100,
	WARNING = 101,
	ERROR = 102
};

class	Server {
	private:
		// config
		int			port_;

		// variables
		int			server_fd_;
		sockaddr_in	addr_;

	public:
		Server(); // Start server with default configurations
		//Server(std::string& fileName); // Start server with configurations from file
		~Server();

};

#endif
