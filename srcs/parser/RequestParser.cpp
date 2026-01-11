#include "RequestParser.h"

RequestParser::RequestParser() {
}

RequestParser::RequestParser(const std::string& raw) {
	(void)raw;
}

RequestParser::RequestParser(const RequestParser& other) {
	(void)other;
}

RequestParser& RequestParser::operator=(const RequestParser& other) {
	(void)other;
	return *this;
}

RequestParser::~RequestParser() {
}

void RequestParser::parse(const std::string& raw) {
	(void)raw;
}

/*
const Request& RequestParser::getRequest() const {
	return void;
}
*/

void RequestParser::print() const {
}

char RequestParser::current() {
	return 0;
}

char RequestParser::next() {
	return 0;
}

bool RequestParser::hasMore() {
	return 0;
}

void RequestParser::skipWhitespaces() {
}

std::string RequestParser::readUntil(char delimiter) {
	(void)delimiter;
	return "";
}

std::string RequestParser::readLine() {
	return "";
}

std::string RequestParser::trim(const std::string& str) {
	(void)str;
	return "";
}

void RequestParser::parseRequestLine() {
}

void RequestParser::parseHeaders() {
}

void RequestParser::parseBody() {
}

void RequestParser::parseCookies(const std::string& cookieHeader) {
	(void)cookieHeader;
}

void RequestParser::parseSetCookie(const std::string& setCookieHeader) {
	(void)setCookieHeader;
}
