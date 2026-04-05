#include "ConfigParser.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fstream>

static int g_tests_passed = 0;
static int g_tests_failed = 0;

static void test_assert(bool condition, const char *test_name)
{
	if (condition) {
		std::cout << "[PASS] " << test_name << std::endl;
		g_tests_passed++;
	} else {
		std::cout << "[FAIL] " << test_name << std::endl;
		g_tests_failed++;
	}
}

static void write_config(const char *filename, const char *content)
{
	std::ofstream file(filename);
	file << content;
	file.close();
}

/* Test 1: Parse single server block */
static void test_single_server(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    server_name localhost;\n"
		"    root /var/www;\n"
		"}\n";

	write_config("/tmp/test1.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test1.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers.size() == 1, "single_server: one server parsed");
}

/* Test 2: Parse port number */
static void test_port_parsing(void)
{
	const char *cfg =
		"server {\n"
		"    listen 9090;\n"
		"}\n";

	write_config("/tmp/test2.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test2.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].port == 9090, "port_parsing: port is 9090");
}

/* Test 3: Parse multiple server names */
static void test_multiple_server_names(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    server_name localhost example.com www.example.com;\n"
		"}\n";

	write_config("/tmp/test3.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test3.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].serverNames.size() == 3,
		"multiple_server_names: three names parsed");
	test_assert(config.servers[0].serverNames[0] == "localhost",
		"multiple_server_names: first name is localhost");
	test_assert(config.servers[0].serverNames[2] == "www.example.com",
		"multiple_server_names: third name is www.example.com");
}

/* Test 4: Parse root directive */
static void test_root_directive(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    root /var/www/html;\n"
		"}\n";

	write_config("/tmp/test4.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test4.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].root == "/var/www/html",
		"root_directive: root path parsed");
}

/* Test 5: Parse error pages */
static void test_error_pages(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    error_page 404 /errors/404.html;\n"
		"    error_page 500 /errors/500.html;\n"
		"}\n";

	write_config("/tmp/test5.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test5.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].errorPages.size() == 2,
		"error_pages: two error pages parsed");
	test_assert(config.servers[0].errorPages.at(404) == "/errors/404.html",
		"error_pages: 404 path correct");
	test_assert(config.servers[0].errorPages.at(500) == "/errors/500.html",
		"error_pages: 500 path correct");
}

/* Test 6: Parse client_max_body_size in bytes */
static void test_body_size_bytes(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    client_max_body_size 1024;\n"
		"}\n";

	write_config("/tmp/test6.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test6.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].clientMaxBodySize == 1024,
		"body_size_bytes: 1024 bytes parsed");
}

/* Test 7: Parse client_max_body_size in KB */
static void test_body_size_kb(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    client_max_body_size 512KB;\n"
		"}\n";

	write_config("/tmp/test7.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test7.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].clientMaxBodySize == 512 * 1024,
		"body_size_kb: 512KB parsed");
}

/* Test 8: Parse client_max_body_size in MB */
static void test_body_size_mb(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    client_max_body_size 2MB;\n"
		"}\n";

	write_config("/tmp/test8.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test8.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].clientMaxBodySize == 2 * 1024 * 1024,
		"body_size_mb: 2MB parsed");
}

/* Test 9: Parse location block */
static void test_location_block(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    location / {\n"
		"        root /var/www;\n"
		"    }\n"
		"}\n";

	write_config("/tmp/test9.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test9.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].locations.size() == 1,
		"location_block: one location parsed");
	test_assert(config.servers[0].locations[0].path == "/",
		"location_block: path is /");
}

/* Test 10: Parse location methods */
static void test_location_methods(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    location /api {\n"
		"        methods GET POST DELETE;\n"
		"    }\n"
		"}\n";

	write_config("/tmp/test10.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test10.conf");

	const Config& config = parser.getConfig();
	const LocationConfig& loc = config.servers[0].locations[0];
	test_assert(loc.methods.size() == 3, "location_methods: three methods");
	test_assert(loc.methods[0] == "GET", "location_methods: first is GET");
	test_assert(loc.methods[1] == "POST", "location_methods: second is POST");
	test_assert(loc.methods[2] == "DELETE", "location_methods: third is DELETE");
}

/* Test 11: Parse location autoindex */
static void test_location_autoindex(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    location / {\n"
		"        autoindex on;\n"
		"    }\n"
		"    location /api {\n"
		"        autoindex off;\n"
		"    }\n"
		"}\n";

	write_config("/tmp/test11.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test11.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].locations[0].autoindex == true,
		"location_autoindex: first is on");
	test_assert(config.servers[0].locations[1].autoindex == false,
		"location_autoindex: second is off");
}

/* Test 12: Parse location redirect */
static void test_location_redirect(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    location /old {\n"
		"        redirectCode 301;\n"
		"        redirectUrl http://example.com/new;\n"
		"    }\n"
		"}\n";

	write_config("/tmp/test12.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test12.conf");

	const Config& config = parser.getConfig();
	const LocationConfig& loc = config.servers[0].locations[0];
	test_assert(loc.redirectCode == 301, "location_redirect: code is 301");
	test_assert(loc.redirectUrl == "http://example.com/new",
		"location_redirect: url correct");
}

/* Test 13: Parse location CGI */
static void test_location_cgi(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    location / {\n"
		"        cgi {\n"
		"            .php /usr/bin/php-cgi;\n"
		"            .py /usr/bin/python;\n"
		"        }\n"
		"    }\n"
		"}\n";

	write_config("/tmp/test13.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test13.conf");

	const Config& config = parser.getConfig();
	const LocationConfig& loc = config.servers[0].locations[0];
	test_assert(loc.cgi.size() == 2, "location_cgi: two handlers");
	test_assert(loc.cgi.at(".php") == "/usr/bin/php-cgi",
		"location_cgi: php handler correct");
	test_assert(loc.cgi.at(".py") == "/usr/bin/python",
		"location_cgi: python handler correct");
}

/* Test 14: Parse multiple servers */
static void test_multiple_servers(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    server_name site1.com;\n"
		"}\n"
		"server {\n"
		"    listen 9090;\n"
		"    server_name site2.com;\n"
		"}\n"
		"server {\n"
		"    listen 3000;\n"
		"    server_name site3.com;\n"
		"}\n";

	write_config("/tmp/test14.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test14.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers.size() == 3, "multiple_servers: three servers");
	test_assert(config.servers[0].port == 8080, "multiple_servers: first port 8080");
	test_assert(config.servers[1].port == 9090, "multiple_servers: second port 9090");
	test_assert(config.servers[2].port == 3000, "multiple_servers: third port 3000");
}

/* Test 15: Parse location index files */
static void test_location_index(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    location / {\n"
		"        index index.html index.htm default.html;\n"
		"    }\n"
		"}\n";

	write_config("/tmp/test15.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test15.conf");

	const Config& config = parser.getConfig();
	const LocationConfig& loc = config.servers[0].locations[0];
	test_assert(loc.index.size() == 3, "location_index: three index files");
	test_assert(loc.index[0] == "index.html", "location_index: first is index.html");
	test_assert(loc.index[2] == "default.html", "location_index: third is default.html");
}

/* Test 16: Parse uploadDir */
static void test_location_upload_dir(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    location /upload {\n"
		"        uploadDir /tmp/uploads;\n"
		"    }\n"
		"}\n";

	write_config("/tmp/test16.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test16.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].locations[0].uploadDir == "/tmp/uploads",
		"location_upload_dir: path correct");
}

/* Test 17: Comments are ignored */
static void test_comments_ignored(void)
{
	const char *cfg =
		"# This is a comment\n"
		"server {\n"
		"    # Another comment\n"
		"    listen 8080;\n"
		"    # server_name ignored.com;\n"
		"    server_name example.com;\n"
		"}\n";

	write_config("/tmp/test17.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test17.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].serverNames.size() == 1,
		"comments_ignored: only one server name");
	test_assert(config.servers[0].serverNames[0] == "example.com",
		"comments_ignored: name is example.com");
}

/* Test 18: Parse location with hasClientMaxBodySize */
static void test_location_has_max_body_size(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    location / {\n"
		"        clientMaxBodySize 1MB;\n"
		"        hasClientMaxBodySize true;\n"
		"    }\n"
		"}\n";

	write_config("/tmp/test18.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test18.conf");

	const Config& config = parser.getConfig();
	const LocationConfig& loc = config.servers[0].locations[0];
	test_assert(loc.hasClientMaxBodySize == true,
		"location_has_max_body_size: flag is true");
	test_assert(loc.clientMaxBodySize == 1024 * 1024,
		"location_has_max_body_size: size is 1MB");
}

/* Test 19: Default values */
static void test_default_values(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"}\n";

	write_config("/tmp/test19.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test19.conf");

	const Config& config = parser.getConfig();
	test_assert(config.servers[0].host == "0.0.0.0",
		"default_values: default host is 0.0.0.0");
	test_assert(config.servers[0].clientMaxBodySize == 1024 * 1024,
		"default_values: default body size is 1MB");
}

/* Test 20: Parse allow_methods alias */
static void test_allow_methods_alias(void)
{
	const char *cfg =
		"server {\n"
		"    listen 8080;\n"
		"    location / {\n"
		"        allow_methods GET POST;\n"
		"    }\n"
		"}\n";

	write_config("/tmp/test20.conf", cfg);
	ConfigParser parser;
	parser.parse("/tmp/test20.conf");

	const Config& config = parser.getConfig();
	const LocationConfig& loc = config.servers[0].locations[0];
	test_assert(loc.methods.size() == 2, "allow_methods_alias: two methods");
	test_assert(loc.methods[0] == "GET", "allow_methods_alias: first is GET");
}

int main(void)
{
	std::cout << "=== ConfigParser Tests ===" << std::endl;
	std::cout << std::endl;

	test_single_server();
	test_port_parsing();
	test_multiple_server_names();
	test_root_directive();
	test_error_pages();
	test_body_size_bytes();
	test_body_size_kb();
	test_body_size_mb();
	test_location_block();
	test_location_methods();
	test_location_autoindex();
	test_location_redirect();
	test_location_cgi();
	test_multiple_servers();
	test_location_index();
	test_location_upload_dir();
	test_comments_ignored();
	test_location_has_max_body_size();
	test_default_values();
	test_allow_methods_alias();

	std::cout << std::endl;
	std::cout << "=== Results ===" << std::endl;
	std::cout << "Passed: " << g_tests_passed << std::endl;
	std::cout << "Failed: " << g_tests_failed << std::endl;
	std::cout << "Total:  " << (g_tests_passed + g_tests_failed) << std::endl;

	return (g_tests_failed > 0) ? 1 : 0;
}
