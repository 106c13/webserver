#include "webserv.hpp"
#include <iostream>

#define COLOR_GREEN "\033[1;32m"
#define COLOR_RED   "\033[1;31m"
#define COLOR_RESET "\033[0m"

void	log(int type, const std::string& msg) {
	if (type == INFO) {
		std::cout << COLOR_GREEN
				  << "[INFO] <time> "
				  << msg
				  << COLOR_RESET
				  << std::endl;
	}
	else if (type == ERROR) {
		std::cerr << COLOR_RED
				  << "[ERROR] <time> "
				  << msg
				  << COLOR_RESET
				  << std::endl;
	}
}
