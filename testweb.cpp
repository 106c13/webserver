#include <iostream>
#include <sstream>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <cstdlib>

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
        std::cerr << "❌ Invalid format. Use IP:PORT\n";
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
        herror("socket");
        exit(1);
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        herror("inet_pton");
        close(sock);
        exit(1);
    }

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        herror("connect");
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
            herror("send");
            close(sock);
            exit(1);
        }
        total += sent;
    }
}

std::string readResponse(int sock) {
    char buffer[BUFFER_SIZE];
    std::string response;
    ssize_t bytes;

    while ((bytes = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        response.append(buffer, bytes);
    }

    if (bytes < 0)
        herror("recv");

    return response;
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

bool checkStatus200(const std::string& response) {
    return response.find("200 OK") != std::string::npos;
}

void printResult(const std::string& name, bool pass) {
    if (pass)
        std::cout << "✅ " << name << " → PASS" << std::endl;
    else
        std::cout << "❌ " << name << " → FAIL" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: ./tester IP:PORT\n";
        return 1;
    }

    Target t = parseTarget(argv[1]);

    std::cout << "🚀 Running Webserv Tests\n";
    std::cout << "========================\n" << std::endl;

    // GET Test
    {
        int sock = connectToServer(t.host, t.port);
        std::string req = buildGET(t.host);
        sendRequest(sock, req);
        std::string res = readResponse(sock);
        close(sock);

        printResult("GET /", checkStatus200(res));
    }

    // POST Test
    {
        int sock = connectToServer(t.host, t.port);
        std::string req = buildPOST(t.host);
        sendRequest(sock, req);
        std::string res = readResponse(sock);
        close(sock);

        printResult("POST /", checkStatus200(res));
    }

    std::cout << "\n✨ Done!" << std::endl;
    return 0;
}
