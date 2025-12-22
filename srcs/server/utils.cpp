#include "webserv.hpp"
#include <iostream>

#define COLOR_GREEN "\033[1;32m"
#define COLOR_RED   "\033[1;31m"
#define COLOR_RESET "\033[0m"

void	log(int type, const std::string& msg) {
	if (type == INFO) {
		std::cout << COLOR_GREEN
				  << "[INFO] "
				  << msg
				  << COLOR_RESET
				  << std::endl;
	}
	else if (type == ERROR) {
		std::cerr << COLOR_RED
				  << "[ERROR] "
				  << msg
				  << COLOR_RESET
				  << std::endl;
	}
}

bool fileExists(const std::string& path) {
    return (access(path.c_str(), F_OK) == 0);
}
