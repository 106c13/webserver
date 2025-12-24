#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <string>
#include <map>
#include <vector>

struct HttpRequest {
	std::string							method;      // GET, POST, DELETE
	std::string							path;        // /index.html
	std::string							query;       // id=5&name=test (after ?)
	std::string							version;     // HTTP/1.1

	std::map<std::string, std::string>	headers;

	std::string							body;
	size_t								contentLength;

	bool								isChunked;
	bool								isMultipart;
	std::string							boundary;

	HttpRequest();
};

class HttpRequestParser {
private:
	HttpRequest	_request;

public:
	HttpRequestParser();
	HttpRequestParser(const HttpRequestParser& other);
	HttpRequestParser& operator=(const HttpRequestParser& other);
	~HttpRequestParser();

	const HttpRequest& getRequest() const;
};

#endif

