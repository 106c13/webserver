#ifndef DEFINES_H
#define DEFINES_H

#include "ConfigParser.h"

typedef std::vector<LocationConfig> LocationList;

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
	BAD_REQUEST = 400,
	NOT_FOUND = 404,
	FORBIDDEN = 403,
	SERVER_ERROR = 500
};

#endif
