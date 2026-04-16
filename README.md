*This project has been created as part of the 42 curriculum by haaghaja, azolotar*

## Description

**webserv** is a non-blocking, event-driven HTTP/1.1 server written from
scratch in **C++98** as part of the 42 cursus - no external libraries, no
threads, one event loop handles every client, every upload, and every CGI
child

The goal of the project is to learn, by building, how a real web server
works under the hood: raw BSD sockets, non-blocking I/O multiplexing, the
HTTP/1.1 wire format, CGI/1.1 gateway semantics, and an Nginx-flavoured
configuration language

The server:

- accepts concurrent connections on any number of ports and virtual hosts
- serves static files, auto-generated directory indexes, redirects, and
  custom error pages
- spools fixed-length and chunked request bodies to disk without blocking
- decodes `multipart/form-data` uploads straight into a configurable upload
  directory
- executes CGI scripts in forked children with a full CGI/1.1 environment,
  and reaps runaway scripts on timeout
- runs the same source on **Linux** (`epoll`) and **macOS** (`kqueue`) via a
  thin macro layer in `include/webserv.h`

Supported HTTP methods: **GET**, **POST**, **DELETE**

Supported body framings: fixed `Content-Length` and
`Transfer-Encoding: chunked`

## Instructions

### Requirements

- A C++ compiler supporting **C++98** (`g++` or `clang++`)
- **GNU make**
- Linux or macOS (no code change required between the two)

### Compilation

```bash
make         # build ./webserv
make re      # full rebuild
make clean   # remove obj/
make fclean  # remove obj/ and the ./webserv binary
make test    # build + run the config and request parser unit tests
```

The Makefile builds with `-std=c++98 -Wall -Wextra -Werror`

### Execution

```bash
./webserv <config_file>
```

The bundled `config.conf` starts three servers:

| Port | What it serves                         |
| ---- | -------------------------------------- |
| 8080 | PHP via `/usr/bin/php-cgi`             |
| 8081 | Retro-themed static site + autoindex   |
| 8082 | File-upload playground (1 GB body cap) |

Quick smoke test:

```bash
./webserv config.conf
# in another shell:
curl http://localhost:8081/
curl -F "file=@Makefile" http://localhost:8082/
```

Raw HTTP fixtures are provided in `test_requests/` and can be piped directly
into the server:

```bash
nc localhost 8080 < test_requests/get_simple.txt
nc localhost 8082 < test_requests/file_upload.txt
```

The `YoupiBanane/` directory ships the standard 42 `ubuntu_cgi_tester` tree
and a matching config:

```bash
./webserv YoupiBanane/config.conf
./ubuntu_cgi_tester
```

### Configuration format

Configuration is nginx-inspired: a list of `server { ... }` blocks, each
containing any number of `location { ... }` blocks

`#` starts a line comment

Server-block directives: `listen`, `server_name`, `root`, `error_page`,
`client_max_body_size`, `cgi { method ...; .ext /handler; }`, `location`

Location-block directives: `root`, `index`, `methods` / `allow_methods`,
`autoindex`, `redirectCode`, `redirectUrl`, `uploadDir`, `clientMaxBodySize`,
`hasClientMaxBodySize`, `cgi { .ext /handler; }`

Size values accept the suffixes `B`, `KB`, `MB`, `GB`

```conf
server {
    listen 8081;
    server_name localhost mysite.com;
    root pages/retro/;
    client_max_body_size 1MB;
    error_page 404 errorPages/404.html;
    error_page 500 errorPages/500.html;

    location / {
        root pages/retro;
        methods GET POST;
        index index.html;
        autoindex on;
        uploadDir /tmp/uploads;
        clientMaxBodySize 512KB;
        hasClientMaxBodySize true;
    }
}
```

## Resources

### Classic references

- **RFC 7230** - HTTP/1.1 Message Syntax and Routing
- **RFC 7231** - HTTP/1.1 Semantics and Content
- **RFC 3875** - The Common Gateway Interface (CGI) Version 1.1
- **Beej's Guide to Network Programming** - the canonical intro to BSD sockets
- Linux manual pages: `man 2 socket`, `man 7 epoll`, `man 2 accept`,
  `man 2 recv`, `man 2 fcntl`
- macOS manual pages: `man 2 kqueue`, `man 2 kevent`
- **Nginx documentation** - for the shape of `server` / `location` directives
  and sensible defaults

### AI Usage

AI assistants (ChatGPT, Claude) were used as a reference and rubber-duck
partner, not as a code generator

Specifically:

- **For clarifying HTTP/1.1 edge cases** - chunked transfer framing, header
  folding rules, and how browsers handle `Connection: close` vs keep-alive

  These clarifications informed the body-reading logic in
  `srcs/server/BodyHandler.cpp` and the request parser in
  `srcs/parser/RequestParser.cpp`
- **For debugging sockets and multiplexing** - especially portability
  differences between `epoll` (Linux) and `kqueue` (macOS)

  Helpful while shaping the cross-platform abstraction in
  `include/webserv.h` and the event dispatch in
  `srcs/server/ServerManager.cpp`
- **For CGI/1.1 environment semantics** - when to set `REDIRECT_STATUS`,
  the distinction between `SCRIPT_FILENAME` and `PATH_INFO`, and how CGI
  responses interleave headers and body

  Used while implementing `srcs/server/cgi.cpp`
- **For reviewing the multipart parser and the non-blocking state machine**
  for logical gaps before submission

AI was **not** used to generate the core architecture, the event loop, the
config / request parsers, or the CGI runtime

Those were designed, written, and debugged manually

---

## Authors

- **Arseniy Zolotarev** - `azolotar`
- **Hakob Aghajanyan** - `haaghaja`
