#ifndef WEBSERV_H
#define WEBSERV_H

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <unistd.h>
#include <exception>
#include <dirent.h>
#include <cstddef>
#include <cstring> 
#include "ConfigParser.h"
#include "RequestParser.h"
#include "HeaderGenerator.h"
#include "defines.h"
#include "Buffer.h"

#define COLOR_GREEN "\033[1;32m"
#define COLOR_RED   "\033[1;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_RESET "\033[0m"


struct Connection {
	int		fd;
	Buffer	recvBuffer;
	Buffer	sendBuffer;
	bool	writable;
};


struct DirEntry {
	std::string name;
	bool		is_dir;
};

class	HttpRequest {
	private:
		// I don't know
		int			request_fd_;
		std::string content_;
		Request		req_;
		Response	res_;
	public:
		HttpRequest(int	fd);
		~HttpRequest();

		// Request
		void					setRequest(const Request& request);
		void					setPath(const std::string& file);
		const struct Request&	getRequest();
		const std::string&		getPath() const;
		const std::string&		getMethod() const;
		const std::string&		getURI() const;
		// Response
		void					setStatus(int code);
		void					setContentType(const std::string& s);
		void					setContentLength(size_t len);
		void					setConnectionType(const std::string& s);
		void					setLocation(const std::string& s);
		const struct Response&	getResponse();

		const std::string&		getContent();

		int						sendAll(const std::string& response);
		int						sendAll(const char* buf, size_t len);
		int						sendChunked(const int fd);
		int						sendFile(const std::string& path);
};

class	Server {
	private:
		// config
		ServerConfig	config_;
		RequestParser	parser_;

		// variables
		std::map<int, Connection> connections_;
		int			serverFd_;
		int			epollFd_;
		sockaddr_in	addr_;

		void			initSocket();
		void			acceptConnection();
		void			handleClient(epoll_event& event);
		void			handleRead(Connection& conn);
		void			handleWrite(Connection& conn);
		void			modifyToWrite(int fd);
		void			modifyToRead(int fd);
		void			closeConnection(int fd);
		int				runCGI(const char* path, const char* cgiPath, const HttpRequest& request);
		void			handleRequest(HttpRequest&	request);
		int				resolvePath(std::string& path, LocationConfig& location);
		LocationConfig&	resolveLocation(std::string& fs_path);
		void			generateAutoindex(HttpRequest& request, LocationConfig& location);
		void			sendRedirect(HttpRequest& request, const LocationConfig& location);
		std::string		findCGI(const std::string& fileName, const std::map<std::string, std::string>& cgiMap);
		std::string		findErrorPage(int code) const;

	public:
		Server(const ServerConfig& config); // Start server with configurations from file
		~Server();

		void		sendError(int code, HttpRequest& request) const;
		void		loop();
};

void					log(int type, const std::string& msg);
void					log(const HttpRequest& request);

bool					fileExists(const std::string& path);
bool					canReadFile(const std::string& path);
ssize_t					getFileSize(const std::string& path);
std::string				readFile(const std::string& filename);
std::vector<DirEntry>	listDirectory(const std::string& path);
#endif
