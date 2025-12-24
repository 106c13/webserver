#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <vector>
#include <map>
#include <string>

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
	size_t						clientMaxBodySize;
	std::map<int, std::string>	errorPages;
	std::vector<LocationConfig>	locations;

	ServerConfig();
};

struct Config {
	std::vector<ServerConfig> servers;
};

class ConfigParser {
private:
	Config _conf;
	// be done soon
public:
	ConfigParser();
	ConfigParser(const std::string& filename);
	ConfigParser(const ConfigParser& other);
	ConfigParser& operator=(const ConfigParser& other);
	~ConfigParser();

	void parse(const std::string& filename);
	const Config& getConfig() const;                   // <- main func to get config
};

#endif
