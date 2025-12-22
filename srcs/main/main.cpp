#include <iostream>
#include "webserv.hpp"

int main() {
    try {
        Server  server;
        
        while (1) {
            server.acceptConnection();
        }
    } catch (const std::exception& e) {
        log(ERROR, e.what()); 
    }
    return 0;
}
