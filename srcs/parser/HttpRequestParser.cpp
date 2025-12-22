#include "HttpRequest.h"

HttpRequest::HttpRequest() :
	contentLength(0),
	isChunked(false),
	isMultipart(false)
{
}

HttpRequestParser::HttpRequestParser() {
}

HttpRequestParser::HttpRequestParser(const HttpRequestParser& other) :
	_request(other._request)
{
}

HttpRequestParser& HttpRequestParser::operator=(const HttpRequestParser& other) {
	if (this != &other) {
		_request = other._request;
	}
	return *this;
}

HttpRequestParser::~HttpRequestParser() {
}

const HttpRequest& HttpRequestParser::getRequest() const {
	return _request;
}
