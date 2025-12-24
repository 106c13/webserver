#include "ConfigParser.h"

LocationConfig::LocationConfig() :
	autoindex(false),
	redirectCode(0),
	clientMaxBodySize(0),
	hasClientMaxBodySize(false)
{
}

ServerConfig::ServerConfig() :
	host("0.0.0.0"),
	port(8080),
	clientMaxBodySize(1024 * 1024) {
}

ConfigParser::ConfigParser() {
}

ConfigParser::ConfigParser(const std::string& filename) {
	(void)filename;
}

ConfigParser::ConfigParser(const ConfigParser& other) : _conf(other._conf) {
}

ConfigParser& ConfigParser::operator=(const ConfigParser& other) {
	if (this != &other) {
		_conf = other._conf;
	}
	return *this;
}

ConfigParser::~ConfigParser() {
}

void ConfigParser::parse(const std::string& filename) {
	(void)filename;
}

const Config& ConfigParser::getConfig() const {
	return _conf;
}
