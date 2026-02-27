#include "RequestParser.h"
#include <iostream>
#include <sstream>
#include <cstdlib>

/* Cookie */
Cookie::Cookie() : name(""), value("") {
}

Cookie::Cookie(const std::string& n, const std::string& v) : name(n), value(v) {
}

/* SetCookie */
SetCookie::SetCookie() : secure(false), httpOnly(false) {
}

/* MultipartPart */
MultipartPart::MultipartPart() {
}

/* Request */
Request::Request() {
}

/* Class management */
RequestParser::RequestParser() : _pos(0) {
}

RequestParser::RequestParser(const std::string& raw) : _raw(raw), _pos(0) {
	parse(raw);
}

RequestParser::RequestParser(const RequestParser& other)
	: _raw(other._raw), _pos(other._pos), _request(other._request) {
}

RequestParser& RequestParser::operator=(const RequestParser& other) {
	if (this != &other) {
		_raw = other._raw;
		_pos = other._pos;
		_request = other._request;
	}
	return *this;
}

RequestParser::~RequestParser() {
}

/* Public Methods */
void RequestParser::parse(const std::string& raw) {
	_raw = raw;
	_pos = 0;
	_request = Request();

	parseRequestLine();
	parseHeaders();
	parseBody();
}

const Request& RequestParser::getRequest() const {
	return _request;
}

void RequestParser::print() const {
	std::cout << "=== Parsed Request ===" << std::endl;
	std::cout << "Method: " << _request.method << std::endl;
	std::cout << "URI: " << _request.uri << std::endl;
	std::cout << "Path: " << _request.path << std::endl;
	std::cout << "Query String: " << _request.queryString << std::endl;
	std::cout << "Version: " << _request.version << std::endl;

	std::cout << "\nHeaders:" << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = _request.headers.begin();
		 it != _request.headers.end(); ++it) {
		std::cout << "  " << it->first << ": " << it->second << std::endl;
	}

	std::cout << "\nQuery Parameters:" << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = _request.queryParams.begin();
		 it != _request.queryParams.end(); ++it) {
		std::cout << "  " << it->first << " = " << it->second << std::endl;
	}

	std::cout << "\nCookies:" << std::endl;
	for (size_t i = 0; i < _request.cookies.size(); ++i) {
		std::cout << "  " << _request.cookies[i].name << " = " << _request.cookies[i].value << std::endl;
	}

	std::cout << "\nSet-Cookies:" << std::endl;
	for (size_t i = 0; i < _request.setCookies.size(); ++i) {
		const SetCookie& sc = _request.setCookies[i];
		std::cout << "  " << sc.name << " = " << sc.value;
		if (!sc.path.empty()) std::cout << "; Path=" << sc.path;
		if (!sc.domain.empty()) std::cout << "; Domain=" << sc.domain;
		if (!sc.expires.empty()) std::cout << "; Expires=" << sc.expires;
		if (!sc.maxAge.empty()) std::cout << "; Max-Age=" << sc.maxAge;
		if (!sc.sameSite.empty()) std::cout << "; SameSite=" << sc.sameSite;
		if (sc.secure) std::cout << "; Secure";
		if (sc.httpOnly) std::cout << "; HttpOnly";
		std::cout << std::endl;
	}

	std::cout << "\nMultipart Parts: " << _request.multipartParts.size() << std::endl;
	for (size_t i = 0; i < _request.multipartParts.size(); ++i) {
		const MultipartPart& part = _request.multipartParts[i];
		std::cout << "  [Part " << (i + 1) << "]" << std::endl;
		std::cout << "    Name: " << part.name << std::endl;
		std::cout << "    Filename: " << part.filename << std::endl;
		std::cout << "    Content-Type: " << part.contentType << std::endl;
		std::cout << "    Data size: " << part.data.size() << " bytes" << std::endl;
	}

	std::cout << "\nBody (" << _request.body.size() << " bytes):" << std::endl;
	if (_request.body.size() <= 200) {
		std::cout << _request.body << std::endl;
	} else {
		std::cout << _request.body.substr(0, 200) << "..." << std::endl;
	}
}

/* Private Methods */
char RequestParser::current() {
	if (_pos >= _raw.size()) {
		return '\0';
	}
	return _raw[_pos];
}

char RequestParser::next() {
	if (_pos + 1 >= _raw.size()) {
		return '\0';
	}
	_pos += 1;
	return _raw[_pos];
}

bool RequestParser::hasMore() {
	return _pos < _raw.size();
}

void RequestParser::skipWhitespaces() {
	while (hasMore() && (current() == ' ' || current() == '\t')) {
		_pos += 1;
	}
}

std::string RequestParser::readUntil(char delimiter) {
	std::string result;
	while (hasMore() && current() != delimiter) {
		result += current();
		_pos += 1;
	}
	return result;
}

std::string RequestParser::readLine() {
	std::string line;
	while (hasMore()) {
		char c = current();
		if (c == '\r') {
			_pos += 1;
			if (hasMore() && current() == '\n') {
				_pos += 1;
			}
			break;
		}
		if (c == '\n') {
			_pos += 1;
			break;
		}
		line += c;
		_pos += 1;
	}
	return line;
}

std::string RequestParser::trim(const std::string& str) {
	size_t start = 0;
	size_t end = str.size();

	while (start < end && (str[start] == ' ' || str[start] == '\t')) {
		start++;
	}
	while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\t')) {
		end--;
	}
	return str.substr(start, end - start);
}

void RequestParser::parseRequestLine() {
	std::string line = readLine();
	if (line.empty()) {
		return;
	}

	size_t pos1 = line.find(' ');
	if (pos1 == std::string::npos) {
		return;
	}
	_request.method = line.substr(0, pos1);

	size_t pos2 = line.find(' ', pos1 + 1);
	if (pos2 == std::string::npos) {
		_request.uri = line.substr(pos1 + 1);
		_request.version = "HTTP/1.1";
	} else {
		_request.uri = line.substr(pos1 + 1, pos2 - pos1 - 1);
		_request.version = line.substr(pos2 + 1);
	}

	size_t queryPos = _request.uri.find('?');
	if (queryPos != std::string::npos) {
		_request.path = _request.uri.substr(0, queryPos);
		_request.queryString = _request.uri.substr(queryPos + 1);
		parseQueryString(_request.queryString);
	} else {
		_request.path = _request.uri;
		_request.queryString = "";
	}
}

void RequestParser::parseHeaders() {
	while (hasMore()) {
		std::string line = readLine();
		if (line.empty()) {
			break;
		}

		size_t colonPos = line.find(':');
		if (colonPos == std::string::npos) {
			continue;
		}

		std::string name = line.substr(0, colonPos);
		std::string value = trim(line.substr(colonPos + 1));

		_request.headers[name] = value;

		if (name == "Cookie") {
			parseCookies(value);
		}
		if (name == "Set-Cookie") {
			parseSetCookie(value);
		}
	}

    std::map<std::string, std::string>::iterator it;

    it = _request.headers.find("Content-Type");
    if (it != _request.headers.end()) {
        _request.boundary = extractBoundary(it->second);
    }
}

void RequestParser::parseBody() {
	if (hasMore()) {
		_request.body = _raw.substr(_pos);
	}

	if (!_request.boundary.empty()) {
		parseMultipart();
	}
}

void RequestParser::parseCookies(const std::string& cookieHeader) {
	std::string remaining = cookieHeader;

	while (!remaining.empty()) {
		size_t semiPos = remaining.find(';');
		std::string pair;
		if (semiPos != std::string::npos) {
			pair = remaining.substr(0, semiPos);
			remaining = trim(remaining.substr(semiPos + 1));
		} else {
			pair = remaining;
			remaining = "";
		}

		pair = trim(pair);
		size_t eqPos = pair.find('=');
		if (eqPos != std::string::npos) {
			std::string name = trim(pair.substr(0, eqPos));
			std::string value = trim(pair.substr(eqPos + 1));
			_request.cookies.push_back(Cookie(name, value));
		}
	}
}

void RequestParser::parseSetCookie(const std::string& setCookieHeader) {
	SetCookie sc;

	std::string remaining = setCookieHeader;
	bool first = true;

	while (!remaining.empty()) {
		size_t semiPos = remaining.find(';');
		std::string part;
		if (semiPos != std::string::npos) {
			part = remaining.substr(0, semiPos);
			remaining = trim(remaining.substr(semiPos + 1));
		} else {
			part = remaining;
			remaining = "";
		}

		part = trim(part);
		size_t eqPos = part.find('=');

		if (first) {
			if (eqPos != std::string::npos) {
				sc.name = trim(part.substr(0, eqPos));
				sc.value = trim(part.substr(eqPos + 1));
			}
			first = false;
		} else {
			if (eqPos != std::string::npos) {
				std::string attrName = trim(part.substr(0, eqPos));
				std::string attrValue = trim(part.substr(eqPos + 1));

				if (attrName == "Path") {
					sc.path = attrValue;
				} else if (attrName == "Domain") {
					sc.domain = attrValue;
				} else if (attrName == "Expires") {
					sc.expires = attrValue;
				} else if (attrName == "Max-Age") {
					sc.maxAge = attrValue;
				} else if (attrName == "SameSite") {
					sc.sameSite = attrValue;
				}
			} else {
				if (part == "Secure") {
					sc.secure = true;
				} else if (part == "HttpOnly") {
					sc.httpOnly = true;
				}
			}
		}
	}

	_request.setCookies.push_back(sc);
}

void RequestParser::parseQueryString(const std::string& queryString) {
	std::string remaining = queryString;

	while (!remaining.empty()) {
		size_t ampPos = remaining.find('&');
		std::string pair;
		if (ampPos != std::string::npos) {
			pair = remaining.substr(0, ampPos);
			remaining = remaining.substr(ampPos + 1);
		} else {
			pair = remaining;
			remaining = "";
		}

		size_t eqPos = pair.find('=');
		if (eqPos != std::string::npos) {
			std::string key = pair.substr(0, eqPos);
			std::string value = pair.substr(eqPos + 1);
			_request.queryParams[key] = value;
		} else if (!pair.empty()) {
			_request.queryParams[pair] = "";
		}
	}
}

std::string RequestParser::extractBoundary(const std::string& contentType) {
	size_t pos = contentType.find("boundary=");
	if (pos == std::string::npos) {
		return "";
	}
	return contentType.substr(pos + 9);
}

void RequestParser::parseMultipart() {
	if (_request.boundary.empty() || _request.body.empty()) {
		return;
	}

	std::string delimiter = "--" + _request.boundary;
	std::string endDelimiter = delimiter + "--";

	size_t pos = 0;
	size_t bodyLen = _request.body.size();

	pos = _request.body.find(delimiter, pos);
	if (pos == std::string::npos) {
		return;
	}
	pos += delimiter.size();

	while (pos < bodyLen) {
		if (pos < bodyLen && _request.body[pos] == '\r') {
			pos++;
		}
		if (pos < bodyLen && _request.body[pos] == '\n') {
			pos++;
		}

		if (_request.body.substr(pos, 2) == "--") {
			break;
		}

		MultipartPart part;

		while (pos < bodyLen) {
			size_t lineEnd = _request.body.find("\r\n", pos);
			if (lineEnd == std::string::npos) {
				lineEnd = _request.body.find("\n", pos);
			}
			if (lineEnd == std::string::npos) {
				break;
			}

			std::string line = _request.body.substr(pos, lineEnd - pos);
			if (line.empty() || line == "\r") {
				pos = lineEnd + 1;
				if (pos < bodyLen && _request.body[pos - 1] == '\r') {
					pos++;
				}
				break;
			}

			size_t colonPos = line.find(':');
			if (colonPos != std::string::npos) {
				std::string headerName = line.substr(0, colonPos);
				std::string headerValue = trim(line.substr(colonPos + 1));
				part.headers[headerName] = headerValue;

				if (headerName == "Content-Disposition") {
					size_t namePos = headerValue.find("name=\"");
					if (namePos != std::string::npos) {
						namePos += 6;
						size_t nameEnd = headerValue.find("\"", namePos);
						if (nameEnd != std::string::npos) {
							part.name = headerValue.substr(namePos, nameEnd - namePos);
						}
					}
					size_t filePos = headerValue.find("filename=\"");
					if (filePos != std::string::npos) {
						filePos += 10;
						size_t fileEnd = headerValue.find("\"", filePos);
						if (fileEnd != std::string::npos) {
							part.filename = headerValue.substr(filePos, fileEnd - filePos);
						}
					}
				}
				if (headerName == "Content-Type") {
					part.contentType = headerValue;
				}
			}

			pos = lineEnd + 1;
			if (pos < bodyLen && _request.body[pos - 1] == '\r') {
				pos++;
			}
		}

		size_t nextDelim = _request.body.find(delimiter, pos);
		if (nextDelim == std::string::npos) {
			part.data = _request.body.substr(pos);
			_request.multipartParts.push_back(part);
			break;
		}

		size_t dataEnd = nextDelim;
		if (dataEnd > 0 && _request.body[dataEnd - 1] == '\n') dataEnd--;
		if (dataEnd > 0 && _request.body[dataEnd - 1] == '\r') dataEnd--;

		part.data = _request.body.substr(pos, dataEnd - pos);
		_request.multipartParts.push_back(part);

		pos = nextDelim + delimiter.size();
	}
}
