#include <iostream>
#include <sys/wait.h>
#include "webserv.h"

int main() {
    try {
        Server  server;
        int i = 0;
        while (i++ < 10) {
            server.acceptConnection();
            while (waitpid(-1, NULL, WNOHANG) > 0) {}
        }
    } catch (const std::exception& e) {
        log(ERROR, e.what()); 
    }
    return 0;
}
