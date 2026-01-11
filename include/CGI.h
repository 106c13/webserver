#ifndef CGI_H
#define CGI_H

#include "webserv.h"

class CGI {
	private:
		std::string	path_;
	public:
		CGI(std::string path);
		~CGI();

};

#endif
