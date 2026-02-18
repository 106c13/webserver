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

struct Connection {
    int         fd;

    Buffer		recvBuffer;
    Buffer		sendBuffer;

    Request		req;
    Response	res;

	bool		writable;	

	int			fileFd;
	bool		sendingFile;
};


struct DirEntry {
	int			type;
	std::string name;
	int			size;
	bool		is_dir;
};

class	Server {
	private:
		// config
		ServerConfig	config_;
		RequestParser	parser_;

		// variables
		std::map<int, Connection>	connections_;
		int							serverFd_;
		int							epollFd_;
		sockaddr_in					addr_;

		void			initSocket();
		void			acceptConnection();
		void			handleClient(epoll_event& event);
		void			handleRead(Connection& conn);
		void			handleWrite(Connection& conn);
		void			modifyToWrite(int fd);
		void			modifyToRead(int fd);
		void			closeConnection(int fd);
		int				runCGI(const char* path, const char* cgiPath, Connection& conn);
		void			sendCGIOutput(Connection& conn, int cgiFd);
		void			handleRequest(Connection& conn);
		int				resolvePath(std::string& path, LocationConfig& location);
		LocationConfig&	resolveLocation(std::string& fs_path);
		void			generateAutoindex(Connection& conn, LocationConfig& location);
		void			sendRedirect(Connection& conn, const LocationConfig& location);
		std::string		findCGI(const std::string& fileName, const StringMap& cgiMap);
		void			sendFile(Connection& conn, const std::string& path);
		void			sendError(int code, Connection& conn);

	public:
		Server(const ServerConfig& config); // Start server with configurations from file
		~Server();

		void		loop();
};

void					log(int type, const std::string& msg);

bool					fileExists(const std::string& path);
bool					canReadFile(const std::string& path);
ssize_t					getFileSize(const std::string& path);
std::string				readFile(const std::string& filename);
std::vector<DirEntry>	listDirectory(const std::string& path);
std::string				toString(size_t n);
#endif
