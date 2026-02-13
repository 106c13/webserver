#include <dirent.h>
#include "webserv.h"

void Server::generateAutoindex(Connection& conn, LocationConfig& location) {
    Request& req = conn.req;
    Response& res = conn.res;
    std::string path = req.uri;
    std::string page;
    std::vector<DirEntry> dirs;
    std::string header;

    if (path[path.length() - 1] != '/')
        path += '/';

    page += "<!DOCTYPE html>\n";
    page += "<html>\n";
    page += "<head>\n";
    page += "  <meta charset=\"UTF-8\">\n";
    page += "  <title>Index of " + path + "</title>\n";
    page += "</head>\n";
    page += "<body>\n";
    page += "  <h1>Index of " + path + "</h1>\n";
    page += "  <hr>\n";

    dirs = listDirectory(location.root + path);
    for (std::vector<DirEntry>::iterator it = dirs.begin();
         it != dirs.end();
         it++) {
        page += "<a href=\"" + path + it->name + "\">" + it->name + "</a><br>\n";
    }
    page += "</body></html>";
    res.contentLength = toString(page.size());
    res.contentType = "text/html";
	header = generateHeader(res);
    conn.sendBuffer.append(header);
    conn.sendBuffer.append(page);
    
    modifyToWrite(conn.fd);
}
