#include "ConfigParser.h"
#include <fstream>
#include <cctype>
#include <stdexcept>

/* Location Config */
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

/* Class managment */
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

/* Public Methods */
void ConfigParser::parse(const std::string& filename) {
	tokenize(filename);
	_tokens_pos = 0;
	return;

	while (_tokens_pos < _tokens.size()) {
		if (current().value == "server") {
			parseServer();
		} else {
			throw std::runtime_error("Unexpected token: " + current().value);
		}
	}
}

const Config& ConfigParser::getConfig() const {
	return _conf;
}

/* Private Methods */
Token& ConfigParser::current() {
	return _tokens[_tokens_pos];
}

void ConfigParser::tokenize(const std::string& filename) {
	std::string			content;
	std::string			line;
	std::string			word;
	char				c;
	std::ifstream		file(filename.c_str());

	_tokens.clear();
	if (!file.is_open()) {
		throw std::runtime_error("Cannot open config file");
	}
	while (std::getline(file, line)) {
		content += line;
		content += '\n';
	}
	for (size_t i = 0; i < content.size(); ) {
		c = content[i];

		if (std::isspace(c)) {
			i += 1;
			continue;
		}

		if (c == '#') {
			while (i < content.size() && content[i] != '\n') {
				i += 1;
			}
			continue;
		}

		if (c == '}') {
			_tokens.push_back(Token(TOK_RBRACE, "}"));
			i += 1;
			continue;
		}

		if (c == '{') {
			_tokens.push_back(Token(TOK_LBRACE, "{"));
			i += 1;
			continue;
		}

		if (c == ';') {
			_tokens.push_back(Token(TOK_SEMICOLON, ";"));
			i += 1;
			continue;
		}
		word = "";
		while (i < content.size()) {
			c = content[i];
			if (std::isspace(c) || c == '{' || c == '}' || c == ';' || c == '#') {
				break;
			}
			word += c;
			i += 1;
		}
		if (!word.empty()) {
			_tokens.push_back(Token(TOK_WORD, word));
		}
	}
}

void ConfigParser::parseServer() {
}
