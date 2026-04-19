#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>

#define BUFFER_SIZE 4096

struct Target {
    std::string host;
    int port;
};

struct Ports {
    int php;
    int retro;
    int upload;
};

static int g_passed = 0;
static int g_failed = 0;
static std::vector<std::string> g_failed_names;

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

Ports derivePorts(int base) {
    Ports p;
    p.php = base;
    p.retro = base + 1;
    p.upload = base + 2;
    return p;
}

int connectToServer(const std::string& host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "socket: " << std::strerror(errno) << std::endl;
        exit(1);
    }

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "inet_pton: invalid address" << std::endl;
        close(sock);
        exit(1);
    }

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "connect: " << std::strerror(errno) << std::endl;
        close(sock);
        exit(1);
    }

    return sock;
}

bool canConnect(const std::string& host, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return false;

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        close(sock);
        return false;
    }

    bool ok = connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0;
    close(sock);
    return ok;
}

/* Returns true if the full request was sent. Returns false if the peer
   closed early (e.g. server returned 413 and shut down the connection
   before we finished uploading) — caller should still try to read the
   response. Does NOT exit the process. */
bool sendRequest(int sock, const std::string& request) {
    size_t total = 0;

    while (total < request.size()) {
        ssize_t sent = send(sock,
                            request.c_str() + total,
                            request.size() - total,
                            0);
        if (sent <= 0) {
            if (errno != EPIPE && errno != ECONNRESET)
                std::cerr << "send: " << std::strerror(errno) << std::endl;
            return false;
        }
        total += sent;
    }
    return true;
}

std::string readResponse(int sock) {
    char buffer[BUFFER_SIZE];
    std::string response;
    ssize_t bytes;

    while ((bytes = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        response.append(buffer, bytes);
    }

    if (bytes < 0 && errno != ECONNRESET)
        std::cerr << "recv: " << std::strerror(errno) << std::endl;

    return response;
}

/* Read one full response from a keep-alive socket: headers, then exactly
   Content-Length body bytes. Returns empty string on failure. */
std::string readOneResponse(int sock) {
    std::string data;
    char buffer[BUFFER_SIZE];

    while (data.find("\r\n\r\n") == std::string::npos) {
        ssize_t bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0)
            return data;
        data.append(buffer, bytes);
    }

    size_t headerEnd = data.find("\r\n\r\n") + 4;
    size_t contentLength = 0;
    size_t clPos = data.find("Content-Length:");
    if (clPos != std::string::npos && clPos < headerEnd) {
        size_t valStart = clPos + std::strlen("Content-Length:");
        contentLength = std::atoi(data.c_str() + valStart);
    }

    while (data.size() < headerEnd + contentLength) {
        ssize_t bytes = recv(sock, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0)
            break;
        data.append(buffer, bytes);
    }

    return data;
}

int parseStatus(const std::string& response) {
    size_t sp = response.find(' ');
    if (sp == std::string::npos)
        return 0;
    return std::atoi(response.c_str() + sp + 1);
}

std::string getHeader(const std::string& response, const std::string& name) {
    size_t headerEnd = response.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        headerEnd = response.size();

    std::string needle = name + ":";
    size_t pos = 0;
    while (pos < headerEnd) {
        size_t lineEnd = response.find("\r\n", pos);
        if (lineEnd == std::string::npos || lineEnd > headerEnd)
            break;
        std::string line = response.substr(pos, lineEnd - pos);
        if (line.size() >= needle.size()) {
            bool match = true;
            for (size_t i = 0; i < needle.size(); ++i) {
                char a = line[i];
                char b = needle[i];
                if (a >= 'A' && a <= 'Z') a = a - 'A' + 'a';
                if (b >= 'A' && b <= 'Z') b = b - 'A' + 'a';
                if (a != b) { match = false; break; }
            }
            if (match) {
                std::string value = line.substr(needle.size());
                size_t start = value.find_first_not_of(" \t");
                if (start == std::string::npos) return "";
                return value.substr(start);
            }
        }
        pos = lineEnd + 2;
    }
    return "";
}

std::string getBody(const std::string& response) {
    size_t pos = response.find("\r\n\r\n");
    if (pos == std::string::npos)
        return "";
    return response.substr(pos + 4);
}

bool bodyContains(const std::string& response, const std::string& needle) {
    return getBody(response).find(needle) != std::string::npos;
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

std::string buildDELETE(const std::string& host, const std::string& path) {
    std::string req;
    req += "DELETE " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    req += "Connection: close\r\n";
    req += "\r\n";
    return req;
}

std::string buildRequest(const std::string& method,
                         const std::string& path,
                         const std::string& host,
                         const std::string& extraHeaders,
                         const std::string& body) {
    std::string req;
    req += method + " " + path + " HTTP/1.1\r\n";
    req += "Host: " + host + "\r\n";
    if (!body.empty())
        req += "Content-Length: " + toString(body.size()) + "\r\n";
    req += extraHeaders;
    req += "Connection: close\r\n";
    req += "\r\n";
    req += body;
    return req;
}

std::string doRequest(const std::string& host, int port, const std::string& req) {
    int sock = connectToServer(host, port);
    sendRequest(sock, req);
    std::string res = readResponse(sock);
    close(sock);
    return res;
}

bool checkStatus200(const std::string& response) {
    return response.find("200 OK") != std::string::npos;
}

void printResult(const std::string& name, bool pass) {
    if (pass) {
        std::cout << "[PASS] " << name << std::endl;
        g_passed++;
    } else {
        std::cout << "[FAIL] " << name << std::endl;
        g_failed++;
        g_failed_names.push_back(name);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: ./tester IP:PORT\n";
        return 1;
    }

    /* Server may close the socket early (e.g. 413 before body fully sent).
       Default SIGPIPE would kill us silently — ignore it so send() just
       returns EPIPE and sendRequest() can bail out cleanly. */
    signal(SIGPIPE, SIG_IGN);

    Target t = parseTarget(argv[1]);
    Ports p = derivePorts(t.port);

    std::cout << "=== Webserv Integration Tests ===" << std::endl;
    std::cout << std::endl;

    if (!canConnect(t.host, t.port)) {
        std::cout << "[SKIP] webserv not reachable at " << t.host << ":" << t.port
                  << std::endl;
        return 0;
    }

    // GET Test
    {
        std::string res = doRequest(t.host, p.php, buildGET(t.host));
        printResult("GET /", checkStatus200(res));
    }

    // POST Test
    {
        std::string res = doRequest(t.host, p.php, buildPOST(t.host));
        printResult("POST /", checkStatus200(res));
    }

    // GET /does-not-exist → 404
    {
        std::string req = buildRequest("GET", "/does-not-exist",
                                       t.host, "", "");
        std::string res = doRequest(t.host, p.php, req);
        printResult("GET /does-not-exist → 404", parseStatus(res) == 404);
    }

    // DELETE /files/nope.pdf → 404 or 405 (no crash)
    {
        std::string res = doRequest(t.host, p.php,
                                    buildDELETE(t.host, "/files/nope.pdf"));
        int s = parseStatus(res);
        printResult("DELETE /files/nope.pdf → 404|405", s == 404 || s == 405);
    }

    // CGI: GET /index.php → 200, Content-Type present, body non-empty
    {
        std::string req = buildRequest("GET", "/index.php",
                                       t.host, "", "");
        std::string res = doRequest(t.host, p.php, req);
        bool ok = parseStatus(res) == 200
               && !getHeader(res, "Content-Type").empty()
               && !getBody(res).empty();
        printResult("CGI GET /index.php → 200 + body", ok);
    }

    // CGI: GET /missing.php → 404
    {
        std::string req = buildRequest("GET", "/missing.php",
                                       t.host, "", "");
        std::string res = doRequest(t.host, p.php, req);
        printResult("CGI GET /missing.php → 404", parseStatus(res) == 404);
    }

    // Autoindex: GET / on retro port → 200 + listing marker
    {
        if (!canConnect(t.host, p.retro)) {
            std::cout << "[SKIP] autoindex test (port " << p.retro
                      << " not reachable)" << std::endl;
        } else {
            std::string res = doRequest(t.host, p.retro, buildGET(t.host));
            bool ok = parseStatus(res) == 200
                   && (bodyContains(res, "Index of")
                       || bodyContains(res, "<a href"));
            printResult("Autoindex GET / → listing", ok);
        }
    }

    // Malformed request line → server must survive
    {
        int sock = connectToServer(t.host, p.php);
        std::string junk = "0000XvQ[0vh/6\r\nHost: x\r\n\r\n";
        sendRequest(sock, junk);
        std::string res = readResponse(sock);
        close(sock);
        int s = parseStatus(res);
        bool handled = (s == 400) || res.empty();
        printResult("Malformed request → 400 or close", handled);

        // Server still alive?
        std::string res2 = doRequest(t.host, p.php, buildGET(t.host));
        printResult("Server alive after malformed", checkStatus200(res2));
    }


    std::cout << std::endl;
    std::cout << "=== Results ===" << std::endl;
    std::cout << "Passed: " << g_passed << std::endl;
    std::cout << "Failed: " << g_failed << std::endl;
    if (g_failed > 0) {
        std::cout << "Failed tests:" << std::endl;
        for (size_t i = 0; i < g_failed_names.size(); ++i)
            std::cout << "  - " << g_failed_names[i] << std::endl;
    }
    return g_failed > 0 ? 1 : 0;
}
