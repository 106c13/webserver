#include <iostream>
#include <sys/wait.h>
#include "webserv.h"
#include "ConfigParser.h"

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}
    try {
		ConfigParser parser;
        Server  server;
        
		parser.parse(argv[1]);
		parser.print();
        while (1) {
            server.acceptConnection();
            while (waitpid(-1, NULL, WNOHANG) > 0) {}
        }
    } catch (const std::exception& e) {
        log(ERROR, e.what()); 
    }
    return 0;
}
