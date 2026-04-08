#ifndef WEBSERV_H
#define WEBSERV_H

#include <iostream>
#include <sys/socket.h>
#include <map>
#include <vector>

#ifdef __linux__
#include <sys/epoll.h>
#elif __APPLE__
#include <sys/event.h>
#include <sys/time.h>
#endif

#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <ctime>
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
#define IS_EVENT_READ(e) ((e).events & EPOLLIN)
#define IS_EVENT_WRITE(e) ((e).events & EPOLLOUT)
#elif __APPLE__
typedef struct kevent Event;
#define EVENT_READ EVFILT_READ
#define EVENT_WRITE EVFILT_WRITE
#define EVENT_ADD EV_ADD
#define EVENT_MOD EV_ADD
#define EVENT_DEL EV_DELETE
#define IS_EVENT_READ(e) ((e).filter == EVFILT_READ)
#define IS_EVENT_WRITE(e) ((e).filter == EVFILT_WRITE)
#endif


enum ConnState {
    READING_HEADER = 52,
    READING_BODY = 51,
	SENDING_FILE = 50,
    PROCESSING = 45,
    SENDING_RESPONSE = 44,
    TIMEOUT = 43,
    CLOSED = 42,
	FINISHED = 41
};

struct Connection {
    int         	fd;
    int             port;
	ConnState		state;

    Buffer			buffer;
	int				fileBuffer;

    Request			req;
    Response		res;
    ServerConfig*   config;
	LocationConfig	location;

	time_t			lastActivityTime;

	size_t			chunkSize;
	bool			hasChunkSize;
	std::string		tmpFilePath;
    
    Connection() : fd(-1), port(-1), state(READING_HEADER), fileBuffer(-1), config(NULL), lastActivityTime(std::time(NULL)), chunkSize(0), hasChunkSize(false) {}
};

struct CGIProcess {
    pid_t		pid;
    std::string	tmpFilePath;
    Connection*	conn;
	time_t		startTime;

    CGIProcess(pid_t p, const std::string& path, Connection* c)
        : pid(p), tmpFilePath(path), conn(c), startTime(std::time(NULL)) {}
};

struct DirEntry {
	int			type;
	std::string name;
	int			size;
	bool		is_dir;
};

class	ServerManager {
	private:
		// config
		Config                      fullConfig_;
		std::map<int, std::vector<ServerConfig> > ports_; // port -> list of configs

		// variables
		std::map<int, Connection>	connections_;
		std::vector<CGIProcess>		cgiProcesses_;
		std::map<int, int>          listeningSockets_; // fd -> port
		int							epollFd_;

		void			initSockets();
		void			acceptConnection(int serverFd);

		void			handleClient(Event& event);
		void			handleRead(Connection& conn);
		void			handleWrite(Connection& conn);
		void			closeConnection(int fd);
		void			generateAutoindex(Connection& conn);
		void			sendRedirect(Connection& conn);
		bool			prepareFileResponse(Connection& conn, const std::string& path);
		bool			streamFileChunk(Connection& conn);
		void			sendError(int code, Connection& conn);
		
		void			startBodyReading(Connection& conn);
		void			processBody(Connection& conn);
		void			processFixedBody(Connection& conn);
		void			processChunkedBody(Connection& conn);

		bool			handleMultipartUpload(Connection& conn, LocationConfig& location);
		void			processHeaders(Connection& conn);

		void			checkTimeOuts();
		void			checkCGIProcesses();

		void			handleGet(Connection& conn);
		void			handlePost(Connection& conn);

		void			runCGI(const char* cgiPath, Connection& conn);
		void			handleCGIRead(Connection& conn, const std::string& tmpFilePath);
		void			handleCGIWrite(Connection& conn);

		void			modifyToWrite(int fd);
		void			modifyToRead(int fd);
		void			addEvent(int fd, bool wantRead, bool wantWrite);

        ServerConfig*   findServerConfig(int port, const std::string& host);

	public:
		ServerManager(const Config& config);
		~ServerManager();

		void			run();
};

int						resolvePath(std::string& path, LocationConfig& location);
void					log(int type, const std::string& msg);
std::string				generateRandomName(const std::string& prefix);
LocationConfig&			resolveLocation(std::string& fs_path, LocationList& locations);
ssize_t					getFileSize(const std::string& path);
std::vector<DirEntry>	listDirectory(const std::string& path);
std::string				toString(size_t n);
std::string				findCGI(const std::string& fileName, const StringMap& cgiMap);
int						openTempFile(std::string& path);
#endif
