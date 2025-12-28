#include <iostream>
#include <sys/wait.h>
#include "webserv.h"

int main() {
    try {
        Server  server;
        
        while (1) {
            server.acceptConnection();
            while (waitpid(-1, NULL, WNOHANG) > 0) {}
        }
    } catch (const std::exception& e) {
        log(ERROR, e.what()); 
    }
    return 0;
}
