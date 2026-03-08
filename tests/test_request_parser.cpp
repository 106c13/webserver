#include "RequestParser.h"
#include "HeaderGenerator.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

static int g_tests_passed = 0;
static int g_tests_failed = 0;

static void test_assert(bool condition, const char *test_name) {
	if (condition) {
		std::cout << "[PASS] " << test_name << std::endl;
		g_tests_passed++;
	} else {
		std::cout << "[FAIL] " << test_name << std::endl;
		g_tests_failed++;
	}
}

/* Test 1: Parse GET method */
static void test_get_method(void) {
	const char *raw =
		"GET /index.html HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method == "GET", "get_method: method is GET");
}

/* Test 2: Parse POST method */
static void test_post_method(void) {
	const char *raw =
		"POST /submit HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method == "POST", "post_method: method is POST");
}

/* Test 3: Parse DELETE method */
static void test_delete_method(void) {
	const char *raw =
		"DELETE /files/doc.pdf HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method == "DELETE", "delete_method: method is DELETE");
}

/* Test 4: Parse URI */
static void test_uri_parsing(void) {
	const char *raw =
		"GET /path/to/resource HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.uri == "/path/to/resource", "uri_parsing: uri correct");
	test_assert(req.path == "/path/to/resource", "uri_parsing: path correct");
}

/* Test 5: Parse HTTP version */
static void test_http_version(void) {
	const char *raw =
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.version == "HTTP/1.1", "http_version: version is HTTP/1.1");
}

/* Test 6: Parse headers */
static void test_headers(void) {
	const char *raw =
		"GET / HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"User-Agent: Mozilla/5.0\r\n"
		"Accept: text/html\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.headers.size() == 3, "headers: three headers parsed");
	test_assert(req.headers.at("Host") == "localhost:8080", "headers: Host correct");
	test_assert(req.headers.at("User-Agent") == "Mozilla/5.0", "headers: User-Agent correct");
	test_assert(req.headers.at("Accept") == "text/html", "headers: Accept correct");
}

/* Test 7: Parse query string */
static void test_query_string(void) {
	const char *raw =
		"GET /search?q=test&page=1 HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.queryString == "q=test&page=1", "query_string: query string parsed");
	test_assert(req.path == "/search", "query_string: path without query");
}

/* Test 8: Parse query parameters */
static void test_query_params(void) {
	const char *raw =
		"GET /login?username=admin&password=secret HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.queryParams.size() == 2, "query_params: two params");
	test_assert(req.queryParams.at("username") == "admin", "query_params: username correct");
	test_assert(req.queryParams.at("password") == "secret", "query_params: password correct");
}

/* Test 9: Parse body */
static void test_body(void) {
	const char *raw =
		"POST /submit HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Length: 13\r\n"
		"\r\n"
		"Hello, World!";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.body == "Hello, World!", "body: body content correct");
}

/* Test 10: Parse form body */
static void test_form_body(void) {
	const char *raw =
		"POST /login HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: application/x-www-form-urlencoded\r\n"
		"Content-Length: 29\r\n"
		"\r\n"
		"username=admin&password=12345";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.body == "username=admin&password=12345", "form_body: body correct");
}

/* Test 11: Parse cookies */
static void test_cookies(void) {
	const char *raw =
		"GET /profile HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Cookie: session_id=abc123; theme=dark\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.cookies.size() == 2, "cookies: two cookies parsed");
	test_assert(req.cookies[0].name == "session_id", "cookies: first name correct");
	test_assert(req.cookies[0].value == "abc123", "cookies: first value correct");
	test_assert(req.cookies[1].name == "theme", "cookies: second name correct");
	test_assert(req.cookies[1].value == "dark", "cookies: second value correct");
}

/* Test 12: Parse Set-Cookie */
static void test_set_cookie(void) {
	const char *raw =
		"HTTP/1.1 200 OK\r\n"
		"Set-Cookie: session_id=abc123; Path=/; HttpOnly\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.setCookies.size() == 1, "set_cookie: one set-cookie");
	test_assert(req.setCookies[0].name == "session_id", "set_cookie: name correct");
	test_assert(req.setCookies[0].value == "abc123", "set_cookie: value correct");
	test_assert(req.setCookies[0].path == "/", "set_cookie: path correct");
	test_assert(req.setCookies[0].httpOnly == true, "set_cookie: httpOnly flag");
}

/* Test 13: Parse Set-Cookie with Secure */
static void test_set_cookie_secure(void) {
	const char *raw =
		"HTTP/1.1 200 OK\r\n"
		"Set-Cookie: token=xyz; Secure; SameSite=Strict\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.setCookies[0].secure == true, "set_cookie_secure: Secure flag");
	test_assert(req.setCookies[0].sameSite == "Strict", "set_cookie_secure: SameSite value");
}

/* Test 14: Parse multipart form data */
static void test_multipart(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=----boundary123\r\n"
		"\r\n"
		"------boundary123\r\n"
		"Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"File content here\r\n"
		"------boundary123--\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.multipartParts.size() == 1, "multipart: one part parsed");
	test_assert(req.multipartParts[0].name == "file", "multipart: name is file");
	test_assert(req.multipartParts[0].filename == "test.txt", "multipart: filename correct");
	test_assert(req.multipartParts[0].contentType == "text/plain", "multipart: content-type");
}

/* Test 15: Parse multipart data content */
static void test_multipart_data(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=----bound\r\n"
		"\r\n"
		"------bound\r\n"
		"Content-Disposition: form-data; name=\"field\"\r\n"
		"\r\n"
		"value123\r\n"
		"------bound--\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.multipartParts[0].data == "value123", "multipart_data: data correct");
}

/* Test 16: Parse Authorization header */
static void test_authorization_header(void) {
	const char *raw =
		"DELETE /files/doc.pdf HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Authorization: Bearer token123\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.headers.at("Authorization") == "Bearer token123",
		"authorization_header: value correct");
}

/* Test 17: Parse Content-Type header */
static void test_content_type(void) {
	const char *raw =
		"POST /api HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: application/json\r\n"
		"\r\n"
		"{\"key\":\"value\"}";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.headers.at("Content-Type") == "application/json",
		"content_type: header correct");
	test_assert(req.body == "{\"key\":\"value\"}", "content_type: body is json");
}

/* Test 18: Parse Connection header */
static void test_connection_header(void) {
	const char *raw =
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Connection: keep-alive\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.headers.at("Connection") == "keep-alive",
		"connection_header: keep-alive");
}

/* Test 19: Parse Accept-Language header */
static void test_accept_language(void) {
	const char *raw =
		"GET / HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.headers.at("Accept-Language") == "en-US,en;q=0.5",
		"accept_language: value correct");
}

/* Test 20: Empty query parameter value */
static void test_empty_query_value(void) {
	const char *raw =
		"GET /search?q=&empty HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.queryParams.at("q") == "", "empty_query_value: empty value");
	test_assert(req.queryParams.at("empty") == "", "empty_query_value: no value key");
}

/* Test 21: Parse Set-Cookie with Expires */
static void test_set_cookie_expires(void) {
	const char *raw =
		"HTTP/1.1 200 OK\r\n"
		"Set-Cookie: session=abc; Expires=Wed, 21 Oct 2026 07:28:00 GMT\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.setCookies[0].expires == "Wed, 21 Oct 2026 07:28:00 GMT",
		"set_cookie_expires: expires value");
}

/* Test 22: Parse Set-Cookie with Domain */
static void test_set_cookie_domain(void) {
	const char *raw =
		"HTTP/1.1 200 OK\r\n"
		"Set-Cookie: user=test; Domain=example.com\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.setCookies[0].domain == "example.com",
		"set_cookie_domain: domain value");
}

/* Test 23: Parse Set-Cookie with Max-Age */
static void test_set_cookie_max_age(void) {
	const char *raw =
		"HTTP/1.1 200 OK\r\n"
		"Set-Cookie: token=abc; Max-Age=3600\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.setCookies[0].maxAge == "3600", "set_cookie_max_age: max-age value");
}

/* Test 24: Parse LF line endings */
static void test_lf_line_endings(void) {
	const char *raw =
		"GET /index.html HTTP/1.1\n"
		"Host: localhost\n"
		"\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method == "GET", "lf_line_endings: method parsed");
	test_assert(req.headers.at("Host") == "localhost", "lf_line_endings: header parsed");
}

/* Test 25: Parse PUT method */
static void test_put_method(void) {
	const char *raw =
		"PUT /resource HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n"
		"Updated content";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method == "PUT", "put_method: method is PUT");
	test_assert(req.body == "Updated content", "put_method: body correct");
}

/* Test 26: File upload - extract boundary from Content-Type */
static void test_upload_boundary_extraction(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryXyZ123\r\n"
		"\r\n"
		"------WebKitFormBoundaryXyZ123\r\n"
		"Content-Disposition: form-data; name=\"file\"; filename=\"test.txt\"\r\n"
		"\r\n"
		"data\r\n"
		"------WebKitFormBoundaryXyZ123--\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.boundary == "----WebKitFormBoundaryXyZ123",
		"upload_boundary: boundary extracted correctly");
}

/* Test 27: File upload - binary file content preserved */
static void test_upload_binary_content(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=----bound\r\n"
		"\r\n"
		"------bound\r\n"
		"Content-Disposition: form-data; name=\"file\"; filename=\"image.png\"\r\n"
		"Content-Type: image/png\r\n"
		"\r\n"
		"\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\r\n"
		"------bound--\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.multipartParts.size() == 1, "upload_binary: one part");
	test_assert(req.multipartParts[0].filename == "image.png", "upload_binary: filename");
	test_assert(req.multipartParts[0].contentType == "image/png", "upload_binary: content-type");
	test_assert(req.multipartParts[0].data.size() > 0, "upload_binary: data not empty");
}

/* Test 28: File upload - multiple files */
static void test_upload_multiple_files(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=----bound\r\n"
		"\r\n"
		"------bound\r\n"
		"Content-Disposition: form-data; name=\"file1\"; filename=\"doc.pdf\"\r\n"
		"Content-Type: application/pdf\r\n"
		"\r\n"
		"PDF content here\r\n"
		"------bound\r\n"
		"Content-Disposition: form-data; name=\"file2\"; filename=\"image.jpg\"\r\n"
		"Content-Type: image/jpeg\r\n"
		"\r\n"
		"JPEG content here\r\n"
		"------bound--\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.multipartParts.size() == 2, "upload_multiple: two parts");
	test_assert(req.multipartParts[0].filename == "doc.pdf", "upload_multiple: first filename");
	test_assert(req.multipartParts[0].contentType == "application/pdf", "upload_multiple: first type");
	test_assert(req.multipartParts[0].data == "PDF content here", "upload_multiple: first data");
	test_assert(req.multipartParts[1].filename == "image.jpg", "upload_multiple: second filename");
	test_assert(req.multipartParts[1].contentType == "image/jpeg", "upload_multiple: second type");
	test_assert(req.multipartParts[1].data == "JPEG content here", "upload_multiple: second data");
}

/* Test 29: File upload - mixed fields and files */
static void test_upload_mixed_fields(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=----bound\r\n"
		"\r\n"
		"------bound\r\n"
		"Content-Disposition: form-data; name=\"description\"\r\n"
		"\r\n"
		"My vacation photo\r\n"
		"------bound\r\n"
		"Content-Disposition: form-data; name=\"file\"; filename=\"photo.jpg\"\r\n"
		"Content-Type: image/jpeg\r\n"
		"\r\n"
		"JPEG binary data\r\n"
		"------bound--\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.multipartParts.size() == 2, "upload_mixed: two parts");
	test_assert(req.multipartParts[0].name == "description", "upload_mixed: text field name");
	test_assert(req.multipartParts[0].filename == "", "upload_mixed: text has no filename");
	test_assert(req.multipartParts[0].data == "My vacation photo", "upload_mixed: text data");
	test_assert(req.multipartParts[1].name == "file", "upload_mixed: file field name");
	test_assert(req.multipartParts[1].filename == "photo.jpg", "upload_mixed: file has filename");
}

/* Test 30: File upload - large file simulation */
static void test_upload_large_content(void) {
	std::string largeData(1024, 'X');  // 1KB of X
	std::string raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=----bound\r\n"
		"\r\n"
		"------bound\r\n"
		"Content-Disposition: form-data; name=\"bigfile\"; filename=\"large.bin\"\r\n"
		"Content-Type: application/octet-stream\r\n"
		"\r\n" + largeData + "\r\n"
		"------bound--\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.multipartParts[0].data.size() == 1024, "upload_large: data size is 1024");
	test_assert(req.multipartParts[0].data == largeData, "upload_large: data matches");
}

/* Test 31: File upload - special characters in filename */
static void test_upload_special_filename(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=----bound\r\n"
		"\r\n"
		"------bound\r\n"
		"Content-Disposition: form-data; name=\"file\"; filename=\"my file (1).txt\"\r\n"
		"\r\n"
		"content\r\n"
		"------bound--\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.multipartParts[0].filename == "my file (1).txt",
		"upload_special_filename: spaces and parens preserved");
}

/* Test 32: File download request */
static void test_download_request(void) {
	const char *raw =
		"GET /files/report.pdf HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Accept: application/pdf\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method == "GET", "download_request: method is GET");
	test_assert(req.path == "/files/report.pdf", "download_request: path correct");
	test_assert(req.headers.at("Accept") == "application/pdf", "download_request: accept header");
}

/* Test 33: Response Content-Disposition header for download */
static void test_response_content_disposition(void) {
	Response res;
	res.status = OK;
	res.contentType = "application/octet-stream";
	res.contentLength = "12345";
	res.contentDisposition = "attachment; filename=\"report.pdf\"";

	std::string headerStr = generateHeader(res);

	test_assert(headerStr.find("Content-Disposition: attachment; filename=\"report.pdf\"") != std::string::npos,
		"response_disposition: header contains Content-Disposition");
	test_assert(headerStr.find("Content-Type: application/octet-stream") != std::string::npos,
		"response_disposition: header contains Content-Type");
	test_assert(headerStr.find("Content-Length: 12345") != std::string::npos,
		"response_disposition: header contains Content-Length");
}

/* Test 34: Response without Content-Disposition */
static void test_response_no_disposition(void) {
	Response res;
	res.status = OK;
	res.contentType = "text/html";
	res.contentLength = "100";

	std::string headerStr = generateHeader(res);

	test_assert(headerStr.find("Content-Disposition") == std::string::npos,
		"response_no_disposition: no Content-Disposition when empty");
}

/* Test 35: Response inline disposition for preview */
static void test_response_inline_disposition(void) {
	Response res;
	res.status = OK;
	res.contentType = "application/pdf";
	res.contentLength = "5000";
	res.contentDisposition = "inline; filename=\"preview.pdf\"";

	std::string headerStr = generateHeader(res);

	test_assert(headerStr.find("Content-Disposition: inline; filename=\"preview.pdf\"") != std::string::npos,
		"response_inline: inline disposition works");
}

/* ============================================ */
/* Malformed Request Tests (Segfault Prevention) */
/* ============================================ */

/* Test 36: Garbage request line (like the reported segfault case) */
static void test_malformed_garbage_request_line(void) {
	const char *raw =
		"0000XvQ[0vh/6\r\n"
		"Host: 127.0.0.1:8080\r\n"
		"User-Agent: Go-http-client/1.1\r\n"
		"Transfer-Encoding: chunked\r\n"
		"Content-Type: test/file\r\n"
		"Accept-Encoding: gzip\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method.empty() || !req.method.empty(),
		"malformed_garbage_line: parser does not crash");
}

/* Test 37: Empty request */
static void test_malformed_empty_request(void) {
	const char *raw = "";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method.empty(), "malformed_empty: method is empty");
	test_assert(req.body.empty(), "malformed_empty: body is empty");
}

/* Test 38: Only CRLF */
static void test_malformed_only_crlf(void) {
	const char *raw = "\r\n\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method.empty(), "malformed_only_crlf: no crash, method empty");
}

/* Test 39: Non-printable characters in request line */
static void test_malformed_non_printable_chars(void) {
	const char raw[] = "\x01\x02\x03 \x04\x05 \x06\x07\x08\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(!req.method.empty(), "malformed_non_printable: method parsed (garbage)");
}

/* Test 40: Null bytes in request */
static void test_malformed_null_bytes(void) {
	std::string raw = "GET /path HTTP/1.1\r\n";
	raw += "Host: local";
	raw += '\0';
	raw += "host\r\n\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method == "GET", "malformed_null_bytes: method parsed correctly");
}

/* Test 41: Truncated request (no CRLF at end) */
static void test_malformed_truncated_no_crlf(void) {
	const char *raw = "GET /index.html HTTP/1.1\r\nHost: localhost";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method == "GET", "malformed_truncated: method parsed");
	test_assert(req.headers.count("Host") == 1, "malformed_truncated: host header exists");
}

/* Test 42: Header with no value (just colon) */
static void test_malformed_header_colon_only(void) {
	const char *raw =
		"GET / HTTP/1.1\r\n"
		"Host:\r\n"
		"Empty-Header:\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.headers.at("Host") == "", "malformed_colon_only: empty host value");
}

/* Test 43: Header with no colon */
static void test_malformed_header_no_colon(void) {
	const char *raw =
		"GET / HTTP/1.1\r\n"
		"InvalidHeaderNoColon\r\n"
		"Host: localhost\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.headers.at("Host") == "localhost", "malformed_no_colon: valid header parsed");
}

/* Test 44: Multipart with empty boundary */
static void test_malformed_multipart_empty_boundary(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=\r\n"
		"\r\n"
		"some body content";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.boundary.empty(), "malformed_empty_boundary: boundary is empty");
	test_assert(req.multipartParts.empty(), "malformed_empty_boundary: no parts parsed");
}

/* Test 45: Multipart with truncated body */
static void test_malformed_multipart_truncated(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=----bound\r\n"
		"\r\n"
		"------bound\r\n"
		"Content-Disposition: form-data; name=\"field\"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.boundary == "----bound", "malformed_truncated_multipart: boundary parsed");
}

/* Test 46: Multipart body starting immediately with boundary (pos=0 edge case) */
static void test_malformed_multipart_boundary_at_start(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=bound\r\n"
		"\r\n"
		"--bound\r\n"
		"\r\n"
		"--bound--";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.boundary == "bound", "malformed_boundary_start: no crash");
}

/* Test 47: Binary garbage as entire request */
static void test_malformed_binary_garbage(void) {
	const char raw[] = "\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR\x00\x00";

	RequestParser parser;
	parser.parse(std::string(raw, sizeof(raw) - 1));
	const Request& req = parser.getRequest();

	test_assert(true, "malformed_binary_garbage: no crash");
	(void)req;
}

/* Test 48: Very long line without CRLF */
static void test_malformed_very_long_line(void) {
	std::string raw = "GET /";
	raw += std::string(10000, 'A');
	raw += " HTTP/1.1\r\n\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method == "GET", "malformed_long_line: method parsed");
	test_assert(req.uri.size() > 1000, "malformed_long_line: long uri parsed");
}

/* Test 49: Request line with only method (no space) */
static void test_malformed_method_only(void) {
	const char *raw = "GET\r\n\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.method.empty(), "malformed_method_only: method empty (no space)");
}

/* Test 50: Multipart with malformed Content-Disposition (no quotes) */
static void test_malformed_content_disposition(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=----bound\r\n"
		"\r\n"
		"------bound\r\n"
		"Content-Disposition: form-data; name=field\r\n"
		"\r\n"
		"value\r\n"
		"------bound--\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.multipartParts.size() == 1, "malformed_disposition: one part parsed");
}

/* Test 51: Control characters in header values */
static void test_malformed_control_chars_header(void) {
	const char raw[] =
		"GET / HTTP/1.1\r\n"
		"Host: local\x01\x02\x03host\r\n"
		"\r\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.headers.count("Host") == 1, "malformed_control_header: header exists");
}

/* Test 52: Multipart with only boundary markers, no content */
static void test_malformed_multipart_only_boundaries(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=b\r\n"
		"\r\n"
		"--b\r\n"
		"--b--";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.boundary == "b", "malformed_only_boundaries: boundary parsed");
}

/* Test 53: Mixed CRLF and LF in multipart */
static void test_malformed_multipart_mixed_lineendings(void) {
	const char *raw =
		"POST /upload HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"Content-Type: multipart/form-data; boundary=----bound\r\n"
		"\r\n"
		"------bound\n"
		"Content-Disposition: form-data; name=\"field\"\n"
		"\n"
		"value\n"
		"------bound--\n";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(req.multipartParts.size() >= 1 || req.multipartParts.empty(),
		"malformed_mixed_lineendings: no crash");
}

/* Test 54: Request with body but no Content-Length or Transfer-Encoding */
static void test_malformed_body_no_length(void) {
	const char *raw =
		"POST /submit HTTP/1.1\r\n"
		"Host: localhost\r\n"
		"\r\n"
		"This is some body data without Content-Length";

	RequestParser parser;
	parser.parse(raw);
	const Request& req = parser.getRequest();

	test_assert(!req.body.empty(), "malformed_body_no_length: body parsed");
}

int main(void) {
	std::cout << "=== RequestParser Tests ===" << std::endl;
	std::cout << std::endl;

	test_get_method();
	test_post_method();
	test_delete_method();
	test_uri_parsing();
	test_http_version();
	test_headers();
	test_query_string();
	test_query_params();
	test_body();
	test_form_body();
	test_cookies();
	test_set_cookie();
	test_set_cookie_secure();
	test_multipart();
	test_multipart_data();
	test_authorization_header();
	test_content_type();
	test_connection_header();
	test_accept_language();
	test_empty_query_value();
	test_set_cookie_expires();
	test_set_cookie_domain();
	test_set_cookie_max_age();
	test_lf_line_endings();
	test_put_method();
	test_upload_boundary_extraction();
	test_upload_binary_content();
	test_upload_multiple_files();
	test_upload_mixed_fields();
	test_upload_large_content();
	test_upload_special_filename();
	test_download_request();
	test_response_content_disposition();
	test_response_no_disposition();
	test_response_inline_disposition();

	std::cout << std::endl;
	std::cout << "=== Malformed Request Tests ===" << std::endl;
	std::cout << std::endl;

	test_malformed_garbage_request_line();
	test_malformed_empty_request();
	test_malformed_only_crlf();
	test_malformed_non_printable_chars();
	test_malformed_null_bytes();
	test_malformed_truncated_no_crlf();
	test_malformed_header_colon_only();
	test_malformed_header_no_colon();
	test_malformed_multipart_empty_boundary();
	test_malformed_multipart_truncated();
	test_malformed_multipart_boundary_at_start();
	test_malformed_binary_garbage();
	test_malformed_very_long_line();
	test_malformed_method_only();
	test_malformed_content_disposition();
	test_malformed_control_chars_header();
	test_malformed_multipart_only_boundaries();
	test_malformed_multipart_mixed_lineendings();
	test_malformed_body_no_length();

	std::cout << std::endl;
	std::cout << "=== Results ===" << std::endl;
	std::cout << "Passed: " << g_tests_passed << std::endl;
	std::cout << "Failed: " << g_tests_failed << std::endl;
	std::cout << "Total:  " << (g_tests_passed + g_tests_failed) << std::endl;

	return (g_tests_failed > 0) ? 1 : 0;
}
