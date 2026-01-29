#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <vector>
#include <map>
#include <string>
#include <stdlib.h>

enum TokenType {
	TOK_WORD,
	TOK_LBRACE,		// {
	TOK_RBRACE,		// }
	TOK_SEMICOLON	// ;
};

struct Token {
	TokenType	type;
	std::string	value;
	Token(TokenType t, const std::string& value) : type(t), value(value) {}
};

struct LocationConfig {
	std::string							path;
	std::string							root;
	std::vector<std::string>			methods;
	std::vector<std::string>			index;
	bool								autoindex;
	int									redirectCode;
	std::string							redirectUrl;
	std::string							uploadDir;
	size_t								clientMaxBodySize;
	bool								hasClientMaxBodySize;
	std::map<std::string, std::string>	cgi;

	LocationConfig();
};

struct ServerConfig {
	std::string					host;
	int							port;
	std::vector<std::string>	serverNames;
	std::string					root;
	long long					clientMaxBodySize;
	std::map<int, std::string>	errorPages;
	std::vector<LocationConfig>	locations;

	ServerConfig();
};

struct Config {
	std::vector<ServerConfig> servers;
};

class ConfigParser {
private:
	Config				_conf;

	std::vector<Token>	_tokens;
	size_t				_tokens_pos;

	Token&		current();
	Token&		next();
	void		expect(TokenType type, const std::string& err);
	void		tokenize(const std::string& filename);
	void		parseServer();
	void		parseLocation(ServerConfig& server);
	long long	parseSize(const std::string& size);
	std::string	stripQuotes(const std::string& str);
public:
	ConfigParser();
	ConfigParser(const std::string& filename);
	ConfigParser(const ConfigParser& other);
	ConfigParser& operator=(const ConfigParser& other);
	~ConfigParser();

	void parse(const std::string& filename);
	const Config& getConfig() const;                   // <- main func to get config
	void print() const;
};

#endif
