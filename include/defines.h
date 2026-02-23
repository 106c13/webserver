#ifndef DEFINES_H
#define DEFINES_H

#include "ConfigParser.h"

typedef std::vector<LocationConfig> LocationList;
typedef std::map<std::string, std::string> StringMap;

#define COLOR_GREEN "\033[1;32m"
#define COLOR_RED "\033[1;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_RESET "\033[0m"

enum Log {
	INFO = 10,
	WARNING = 11,
	ERROR = 12
};

enum Method {
	GET = 358774,
	POST = 101
};

enum Page {
	OK = 200,
	REDIRECT = 301,
	BAD_REQUEST = 400,
	NOT_FOUND = 404,
	FORBIDDEN = 403,
	SERVER_ERROR = 500,
	SERVICE_UNAVAILABLE = 503
};

#endif
