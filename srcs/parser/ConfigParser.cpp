#include "ConfigParser.h"
#include <fstream>
#include <cctype>
#include <stdexcept>
#include <iostream>

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
	root("/tmp/"),
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
	if (_tokens_pos >= _tokens.size()) {
		throw std::runtime_error("Unexpected end of config");
	}
	return _tokens[_tokens_pos];
}

Token& ConfigParser::next() {
	if (_tokens_pos + 1 >= _tokens.size()) {
		throw std::runtime_error("Unexpected end of config");
	}
	_tokens_pos += 1;
	return _tokens[_tokens_pos];
}

void ConfigParser::expect(TokenType type, const std::string& err) {
	if (_tokens_pos >= _tokens.size() || current().type != type) {
		throw std::runtime_error(err);
	}
	_tokens_pos += 1;
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
	ServerConfig server;

	expect(TOK_WORD, "Expected 'server'");
	expect(TOK_LBRACE, "Expected '{' after server");

	while (current().type != TOK_RBRACE) {
		std::string token_value = current().value;
		if (token_value == "listen") {
			next();
			server.port = std::atoi(current().value.c_str());
			expect(TOK_WORD, "Expected port number");
			expect(TOK_SEMICOLON, "Expected ';' after listen");
		} else if (token_value == "server_name") {
			next();
			while (current().type == TOK_WORD) {
				server.serverNames.push_back(current().value);
				next();
			}
			expect(TOK_SEMICOLON, "Expected ';' after server_name");
		} else if (token_value == "root") {
			next();
			server.root = current().value;
			expect(TOK_WORD, "Expected root path");
			expect(TOK_SEMICOLON, "Expected ';' after root");
		} else if (token_value == "error_page") {
			next();
			int code = std::atoi(current().value.c_str());
			expect(TOK_WORD, "Expected error code");

			std::string path = current().value;
			expect(TOK_WORD, "Expected error page path");

			server.errorPages[code] = path;
			expect(TOK_SEMICOLON, "Expected ';' after error_page");
		} else if (token_value == "client_max_body_size") {
			next();
			server.clientMaxBodySize = parseSize(current().value);
			expect(TOK_WORD, "Expected client max body size");
			expect(TOK_SEMICOLON, "Expected ';' after client_max_body_size");
		} else if (token_value == "location") {
			parseLocation(server);
		} else {
			throw std::runtime_error("Unknown directive in server block: " + current().value);
		}
	}
	expect(TOK_RBRACE, "Expected '}' at the end of the server");
	_conf.servers.push_back(server);
}

void ConfigParser::parseLocation(ServerConfig& server) {
	LocationConfig location;

	expect(TOK_WORD, "Expected 'location'");
	location.path = current().value;
	expect(TOK_WORD, "Expected location path");
	expect(TOK_LBRACE, "Expected '{' after location path");

	while (current().type != TOK_RBRACE) {
		std::string directive = current().value;

		if (directive == "root") {
			next();
			location.root = current().value;
			expect(TOK_WORD, "Expected root path");
			expect(TOK_SEMICOLON, "Expected ';' after root");
		} else if (directive == "index") {
			next();
			while (current().type == TOK_WORD) {
				location.index.push_back(current().value);
				next();
			}
			expect(TOK_SEMICOLON, "Expected ';' after index");
		} else if (directive == "methods" || directive == "allow_methods") {
			next();
			while (current().type == TOK_WORD) {
				location.methods.push_back(current().value);
				next();
			}
			expect(TOK_SEMICOLON, "Expected ';' after methods");
		} else if (directive == "autoindex") {
			next();
			std::string value = current().value;
			if (value == "on") {
				location.autoindex = true;
			} else if (value == "off") {
				location.autoindex = false;
			} else {
				throw std::runtime_error("Invalid autoindex value: " + value);
			}
			expect(TOK_WORD, "Expected 'on' or 'off'");
			expect(TOK_SEMICOLON, "Expected ';' after autoindex");
		} else if (directive == "redirectCode") {
			next();
			location.redirectCode = std::atoi(current().value.c_str());
			expect(TOK_WORD, "Expected redirect code");
			expect(TOK_SEMICOLON, "Expected ';' after redirectCode");
		} else if (directive == "redirectUrl") {
			next();
			location.redirectUrl = current().value;
			expect(TOK_WORD, "Expected redirect URL");
			expect(TOK_SEMICOLON, "Expected ';' after redirectUrl");
		} else if (directive == "uploadDir") {
			next();
			location.uploadDir = current().value;
			expect(TOK_WORD, "Expected upload directory path");
			expect(TOK_SEMICOLON, "Expected ';' after uploadDir");
		} else if (directive == "clientMaxBodySize") {
			next();
			location.clientMaxBodySize = parseSize(current().value);
			expect(TOK_WORD, "Expected size value");
			expect(TOK_SEMICOLON, "Expected ';' after clientMaxBodySize");
		} else if (directive == "hasClientMaxBodySize") {
			next();
			std::string value = current().value;
			if (value == "true") {
				location.hasClientMaxBodySize = true;
			} else if (value == "false") {
				location.hasClientMaxBodySize = false;
			} else {
				throw std::runtime_error("Invalid hasClientMaxBodySize value: " + value);
			}
			expect(TOK_WORD, "Expected 'true' or 'false'");
			expect(TOK_SEMICOLON, "Expected ';' after hasClientMaxBodySize");
		} else if (directive == "cgi") {
			next();
			expect(TOK_LBRACE, "Expected '{' after cgi");
			while (current().type != TOK_RBRACE) {
				std::string extension = current().value;
				expect(TOK_WORD, "Expected CGI extension");
				std::string handler = current().value;
				expect(TOK_WORD, "Expected CGI handler path");
				location.cgi[extension] = handler;
				expect(TOK_SEMICOLON, "Expected ';' after cgi entry");
			}
			expect(TOK_RBRACE, "Expected '}' after cgi block");
		} else {
			throw std::runtime_error("Unknown directive in location block: " + directive);
		}
	}
	expect(TOK_RBRACE, "Expected '}' at the end of location");
	server.locations.push_back(location);
}

long long ConfigParser::parseSize(const std::string& size) {
	if (size.empty()) {
		throw std::runtime_error("Empty size value");
	}

	size_t i = 0;

	while (i < size.length() && std::isdigit(size[i])) {
		i++;
	}

	if (i == 0) {
		throw std::runtime_error("Invalid size format: " + size);
	}

	long long value = std::atoll(size.substr(0, i).c_str());
	if (value < 0) {
		throw std::runtime_error("Negative size not allowed: " + size);
	}

	std::string suffix = size.substr(i);
	if (suffix.empty() || suffix == "B") {
		return value;
	}
	if (suffix == "KB") {
		return value * 1024LL;
	}
	if (suffix == "MB") {
		return value * 1024LL * 1024LL;
	}
	if (suffix == "GB") {
		return value * 1024LL * 1024LL * 1024LL;
	}
	throw std::runtime_error("Unknown size suffix: " + size);
}

void ConfigParser::print() const {
	std::cout << "=== Parsed Configuration ===" << std::endl;
	std::cout << "Total servers: " << _conf.servers.size() << std::endl;

	for (size_t i = 0; i < _conf.servers.size(); ++i) {
		const ServerConfig& server = _conf.servers[i];
		std::cout << "\n--- Server " << (i + 1) << " ---" << std::endl;
		std::cout << "  Host: " << server.host << std::endl;
		std::cout << "  Port: " << server.port << std::endl;
		std::cout << "  Root: " << server.root << std::endl;
		std::cout << "  Client Max Body Size: " << server.clientMaxBodySize << std::endl;

		std::cout << "  Server Names: ";
		for (size_t j = 0; j < server.serverNames.size(); ++j) {
			std::cout << server.serverNames[j];
			if (j + 1 < server.serverNames.size()) std::cout << ", ";
		}
		std::cout << std::endl;

		std::cout << "  Error Pages:" << std::endl;
		for (std::map<int, std::string>::const_iterator it = server.errorPages.begin();
			 it != server.errorPages.end(); ++it) {
			std::cout << "    " << it->first << " -> " << it->second << std::endl;
		}

		std::cout << "  Locations: " << server.locations.size() << std::endl;
		for (size_t j = 0; j < server.locations.size(); ++j) {
			const LocationConfig& loc = server.locations[j];
			std::cout << "\n    [Location " << (j + 1) << "] " << loc.path << std::endl;
			std::cout << "      Root: " << loc.root << std::endl;
			std::cout << "      Autoindex: " << (loc.autoindex ? "on" : "off") << std::endl;
			std::cout << "      Upload Dir: " << loc.uploadDir << std::endl;

			if (loc.hasClientMaxBodySize) {
				std::cout << "      Client Max Body Size: " << loc.clientMaxBodySize << std::endl;
			}

			if (loc.redirectCode != 0) {
				std::cout << "      Redirect: " << loc.redirectCode << " -> " << loc.redirectUrl << std::endl;
			}

			std::cout << "      Methods: ";
			for (size_t k = 0; k < loc.methods.size(); ++k) {
				std::cout << loc.methods[k];
				if (k + 1 < loc.methods.size()) std::cout << ", ";
			}
			std::cout << std::endl;

			std::cout << "      Index: ";
			for (size_t k = 0; k < loc.index.size(); ++k) {
				std::cout << loc.index[k];
				if (k + 1 < loc.index.size()) std::cout << ", ";
			}
			std::cout << std::endl;

			std::cout << "      CGI:" << std::endl;
			for (std::map<std::string, std::string>::const_iterator it = loc.cgi.begin();
				 it != loc.cgi.end(); ++it) {
				std::cout << "        " << it->first << " -> " << it->second << std::endl;
			}
		}
	}
}
