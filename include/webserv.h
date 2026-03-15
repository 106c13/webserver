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
    READING_HEADERS = 50,
    READING_BODY = 51,
    READING_CHUNKS = 52,
    READING_CHUNK_SIZE = 53,
    READING_CHUNK_DATA = 54,
    READING_CHUNK_CRLF = 55,
    PROCESSING = 56,
    SENDING_RESPONSE = 57,
    TIMEOUT = 58,
    CLOSED = 59,
    CGI_READING_OUTPUT = 60,
    CGI_WRITING_BODY = 61,
    CGI_BUFFERING_TO_FILE = 62
};

struct Connection {
    int         fd;

    Buffer		recvBuffer;
    Buffer		sendBuffer;

    Request		req;
    Response	res;

	int			fileFd;
	bool		sendingFile;
	
	size_t		remainingBody;
	int			state;

	time_t		lastActivityTime;

	size_t		currentChunkSize;

	int			cgiStdout;
	int			cgiStdin;
	pid_t		cgiPid;
	size_t		cgiWritePos;
	bool		cgiHeaderSent;

	std::string cgiRawOutput;
	std::string	cgiExecPath;
	std::string	cgiBodyTmpPath;
	int			cgiBodyTmpFd;

	int			headerSize;

	LocationConfig	location;
};

struct CGIProcess {
    pid_t pid;
    Connection* conn; 

    CGIProcess(pid_t p, Connection& c) : pid(p), conn(&c) {}
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
		std::map<int, Connection*>	cgiFdMap_;
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
		int				runCGI(const char* cgiPath, Connection& conn);
		void			sendCGIOutput(Connection& conn, int cgiFd);
		void			handleRequest(Connection& conn);
		int				resolvePath(std::string& path, LocationConfig& location);
		LocationConfig&	resolveLocation(std::string& fs_path);
		void			generateAutoindex(Connection& conn, LocationConfig& location);
		void			sendRedirect(Connection& conn, const LocationConfig& location);
		bool			prepareFileResponse(Connection& conn, const std::string& path);
		bool			streamFileChunk(Connection& conn);
		void			sendError(int code, Connection& conn);
		char**			createEnvironment(const Request& req);
		
		void			finishBody(Connection& conn);
		void			processBody(Connection& conn);
		void			processChunkedBody(Connection& conn);
		void			startBodyReading(Connection& conn, size_t endPos);
		void			handleSimpleRequest(Connection& conn, size_t endPos);
		bool			validateRequest(Connection& conn);
		bool			handleMultipartUpload(Connection& conn, LocationConfig& location);
		void			processHeaders(Connection& conn);
		void			checkTimeOuts();

		void			handleCGIRead(Connection& conn);
		void			handleCGIWrite(Connection& conn);
		void			closeCgiFd(int& fd);
		void			checkCGIProcesses();
		void			startQueuedCGIs();

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
#endif
