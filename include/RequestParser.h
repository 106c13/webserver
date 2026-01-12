#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include <string>
#include <map>
#include <vector>

struct Cookie {
	std::string	name;
	std::string	value;
	Cookie();
	Cookie(const std::string& n, const std::string& v);
};

struct SetCookie {
	std::string	name;
	std::string	value;
	std::string	path;
	std::string	domain;
	std::string	expires;
	std::string	maxAge;
	std::string	sameSite;
	bool		secure;
	bool		httpOnly;
	SetCookie();
};

struct MultipartPart {
	std::string							name;
	std::string							filename;
	std::string							contentType;
	std::string							data;
	std::map<std::string, std::string>	headers;
	MultipartPart();
};

struct Request {
	std::string							method;
	std::string							uri;
	std::string							path;
	std::string							queryString;
	std::string							version;
	std::map<std::string, std::string>	headers;
	std::map<std::string, std::string>	queryParams;
	std::string							body;
	std::vector<Cookie>					cookies;
	std::vector<SetCookie>				setCookies;
	std::vector<MultipartPart>			multipartParts;
	std::string							boundary;
	Request();
};

class RequestParser {
public:
	RequestParser();
	RequestParser(const std::string& raw);
	RequestParser(const RequestParser& other);
	RequestParser& operator=(const RequestParser& other);
	~RequestParser();

	void			parse(const std::string& raw);
	void			print() const;
	const Request&	getRequest() const;

private:
	std::string	_raw;
	size_t		_pos;
	Request		_request;

	char			current();
	char			next();
	bool			hasMore();
	void			skipWhitespaces();
	std::string		readUntil(char delimiter);
	std::string		readLine();
	std::string		trim(const std::string& str);

	void			parseRequestLine();
	void			parseHeaders();
	void			parseBody();
	void			parseCookies(const std::string& cookieHeader);
	void			parseSetCookie(const std::string& setCookieHeader);
	void			parseQueryString(const std::string& queryString);
	void			parseMultipart();
	std::string		extractBoundary(const std::string& contentType);
};

#endif
