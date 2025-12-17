#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <iostream>
#include <string>

class	Server {
	private:
		int	port;
	public:
		Server(); // Start server with default configurations
		Server(std::string& fileName); // Start server with configurations from file
		~Server();

		void	listen();
};

#endif
