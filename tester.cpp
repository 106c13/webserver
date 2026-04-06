#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

struct Target {
    std::string host;
    int port;
};

std::string toString(size_t n) {
    std::stringstream ss;
    ss << n;
    return ss.str();
}

Target parseTarget(const std::string& input) {
    size_t pos = input.find(':');
    if (pos == std::string::npos) {
        std::cerr << "Invalid format. Use IP:PORT\n";
        exit(1);
    }

    Target t;
    t.host = input.substr(0, pos);
    t.port = std::atoi(input.substr(pos + 1).c_str());

    return t;
}

int connectToServer(const std::string& host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        exit(1);
    }

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("connect");
        close(sock);
        exit(1);
    }

    return sock;
}

void sendRequest(int sock, const std::string& request) {
    size_t total = 0;

    while (total < request.size()) {
        ssize_t sent = send(sock,
                            request.c_str() + total,
                            request.size() - total,
                            0);
        if (sent <= 0) {
            perror("send");
            close(sock);
            exit(1);
        }
        total += sent;
    }
}

void readResponse(int sock) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes;

    while ((bytes = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes] = '\0';
        std::cout << buffer;
    }

    if (bytes < 0) {
        perror("recv");
    }
}

std::string buildGET(const std::string& host) {
    std::string req;

    req += "GET / HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "Connection: close\r\n";
    req += "\r\n";

    return req;
}

std::string buildPOST(const std::string& host) {
    std::string body = "name=test&value=123";
    std::string req;

    req += "POST / HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "Content-Type: application/x-www-form-urlencoded\r\n";
    req += "Content-Length: " + toString(body.size()) + "\r\n";
    req += "Connection: close\r\n";
    req += "\r\n";
    req += body;

    return req;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: ./tester IP:PORT\n";
        return 1;
    }

    Target target = parseTarget(argv[1]);

    std::cout << "=== GET REQUEST ===\n";
    {
        int sock = connectToServer(target.host, target.port);
        std::string req = buildGET(target.host);
        sendRequest(sock, req);
        readResponse(sock);
        close(sock);
    }

    std::cout << "\n\n=== POST REQUEST ===\n";
    {
        int sock = connectToServer(target.host, target.port);
        std::string req = buildPOST(target.host);
        sendRequest(sock, req);
        readResponse(sock);
        close(sock);
    }

    return 0;
}
