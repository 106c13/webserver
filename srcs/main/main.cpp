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
		parser.parse(argv[1]);
		parser.print();
   		const struct Config config = parser.getConfig();
		Server  server(config.servers[0]);
		int i = 0;
		while (i < 10) {
			try {
				server.acceptConnection();
				i++;
				while (waitpid(-1, NULL, WNOHANG) > 0) {}
            } catch (const std::exception& e) {
				log(ERROR, e.what()); 
			}
		}
	} catch (const std::exception& e) {
		log(ERROR, e.what());
		return 1;
	}
    return 0;
}
