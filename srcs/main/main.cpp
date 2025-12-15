#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "webserv.h"

int main() {
    try {
        Server  server;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl; 
    }
    return 0;
}
