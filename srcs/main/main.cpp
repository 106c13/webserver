#include <iostream>
#include "webserv.hpp"

int main() {
    try {
        Server  server;

        server.acceptConnection();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl; 
    }
    return 0;
}
