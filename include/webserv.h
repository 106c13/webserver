#ifndef WEBSERV_H
#define WEBSERV_H

#include <iostream>
#include <string>
#include <sys/socket.h>
#ifdef __linux__
#include <sys/epoll.h>
#elif __APPLE__
#include <sys/event.h>
#include <sys/time.h>
#endif
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

#ifdef __linux__
typedef struct epoll_event Event;
#define EVENT_READ EPOLLIN
#define EVENT_WRITE EPOLLOUT
#define EVENT_ADD EPOLL_CTL_ADD
#define EVENT_MOD EPOLL_CTL_MOD
#define EVENT_DEL EPOLL_CTL_DEL
#elif __APPLE__
typedef struct kevent Event;
#define EVENT_READ EVFILT_READ
#define EVENT_WRITE EVFILT_WRITE
#define EVENT_ADD EV_ADD
#define EVENT_MOD EV_ADD
#define EVENT_DEL EV_DELETE
#endif

enum ConnState {
	READING_HEADERS = 64,
	READING_BODY = 63,
	PROCESSING = 62,
	SENDING_RESPONSE = 61,
	CLOSED = 60
};

struct Connection {
    int         fd;

    Buffer		recvBuffer;
    Buffer		sendBuffer;

    Request		req;
    Response	res;

	bool		writable;	

	int			fileFd;
	bool		sendingFile;
	
	size_t		remainingBody;
	int			state;
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
		void			handleClient(Event& event);
		void			handleRead(Connection& conn);
		void			handleWrite(Connection& conn);
		void			modifyToWrite(int fd);
		void			modifyToRead(int fd);
		void			closeConnection(int fd);
		int				runCGI(const char* cgiPath, Connection& conn);
		void			sendCGIOutput(Connection& conn, int cgiFd);
		void			handleRequest(Connection& conn);
		int				resolvePath(std::string& path, LocationConfig& location);
		LocationConfig&	resolveLocation(std::string& fs_path);
		void			generateAutoindex(Connection& conn, LocationConfig& location);
		void			sendRedirect(Connection& conn, const LocationConfig& location);
		std::string		findCGI(const std::string& fileName, const StringMap& cgiMap);
		bool			prepareFileResponse(Connection& conn, const std::string& path);
		bool			streamFileChunk(Connection& conn);
		void			sendError(int code, Connection& conn);
		char**			createEnvironment(const Request& req);
		
		void			finishBody(Connection& conn);
		void			processBody(Connection& conn);
		void			startBodyReading(Connection& conn, size_t endPos);
		void			handleSimpleRequest(Connection& conn, size_t endPos);
		bool			validateRequest(Connection& conn);
		void			processHeaders(Connection& conn);

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
