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
#elif __APPLE__
typedef struct kevent Event;
#define EVENT_READ EVFILT_READ
#define EVENT_WRITE EVFILT_WRITE
#define EVENT_ADD EV_ADD
#define EVENT_MOD EV_ADD
#define EVENT_DEL EV_DELETE
#endif


enum ConnState {
    READING_HEADER = 52,
    READING_BODY = 51,
	SENDING_FILE = 50,

    READING_CHUNKS = 49,
    READING_CHUNK_SIZE = 48,
    READING_CHUNK_DATA = 47,
    READING_CHUNK_CRLF = 46,
    PROCESSING = 45,
    SENDING_RESPONSE = 44,
    TIMEOUT = 43,
    CLOSED = 42,
};

struct CGIProcess;

struct Connection {
    int         	fd;
	ConnState		state;

    Buffer			recvBuffer;
    Buffer			sendBuffer;
	int				fileBuffer;

    Request			req;
    Response		res;
	LocationConfig	location;

	time_t			lastActivityTime;

	size_t chunkSize;
	bool   hasChunkSize;
};

struct CGIProcess {
    pid_t pid;
    std::string tmpFilePath;
    Connection* conn;

    CGIProcess(pid_t p, const std::string& path, Connection* c)
        : pid(p), tmpFilePath(path), conn(c) {}
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
		std::vector<CGIProcess>		cgiProcesses_;
		std::vector<int>			cgiQueue_;
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
		void			runCGI(const char* cgiPath, Connection& conn);
		void			sendCGIOutput(Connection& conn, int cgiFd);
		void			handleRequest(Connection& conn);
		int				resolvePath(std::string& path, LocationConfig& location);
		LocationConfig&	resolveLocation(std::string& fs_path);
		void			generateAutoindex(Connection& conn);
		void			sendRedirect(Connection& conn);
		bool			prepareFileResponse(Connection& conn, const std::string& path);
		bool			streamFileChunk(Connection& conn);
		void			sendError(int code, Connection& conn);
		char**			createEnvironment(const Request& req);
		
		void			finishBody(Connection& conn);
		void			processBody(Connection& conn);
		void			processChunkedBody(Connection& conn);
		void			startBodyReading(Connection& conn, size_t endPos);
		bool			handleMultipartUpload(Connection& conn, LocationConfig& location);
		void			processHeaders(Connection& conn);
		void			checkTimeOuts();

		void			handleCGIRead(Connection& conn, const std::string& tmpFilePath);
		void			handleCGIWrite(Connection& conn);
		void			closeCgiFd(int& fd);
		void			checkCGIProcesses();
		void			startQueuedCGIs();
		void			handleGet(Connection& conn);
		void			handlePost(Connection& conn);
		void			startBodyReading(Connection& conn);
		void			addEvent(int fd, bool wantRead, bool wantWrite);
		void			processFixedBody(Connection& conn);

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
std::string				findCGI(const std::string& fileName, const StringMap& cgiMap);
int						openTempFile(std::string& path);
#endif
