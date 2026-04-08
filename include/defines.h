#ifndef DEFINES_H
#define DEFINES_H

#include "ConfigParser.h"

typedef std::vector<LocationConfig> LocationList;
typedef std::map<std::string, std::string> StringMap;

#define COLOR_GREEN "\033[1;32m"
#define COLOR_RED "\033[1;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_RESET "\033[0m"

#define MAX_HEADER_SIZE 8000
#define MAX_MEMORY_BODY_SIZE 1000000
#define MAX_CHUNK_SIZE_LEN 16
#define HEADER_TIMEOUT 10
#define BODY_TIMEOUT 5
#define CGI_TIMEOUT 10

#define BUFFER_SIZE 1024 

enum Log {
	INFO,
	WARNING,
	ERROR,
	DEBUG
};


enum Page {
	OK = 200,
	NO_CONTENT = 204,
	REDIRECT = 301,
	TEMPRORARY_REDIRECT = 302,
	BAD_REQUEST = 400,
	NOT_FOUND = 404,
	FORBIDDEN = 403,
	METHOD_NOT_ALLOWED = 405,
	REQUEST_TIMEOUT = 408,
	LENGTH_REQUIRED = 411,
	PAYLOAD_TOO_LARGE = 413,
	SERVER_ERROR = 500,
	SERVICE_UNAVAILABLE = 503,
	GETWAY_TIMEOUT = 504,
	DIRECTORY_NO_INDEX = 1000
};

#endif
