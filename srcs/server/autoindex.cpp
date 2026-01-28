#include <dirent.h>
#include "webserv.h"

void Server::generateAutoindex(HttpRequest& request, LocationConfig& location) {
    std::string path = request.getURI();
    std::string page;
    std::vector<DirEntry> dirs;
    const char* header;

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
    request.setContentLength(page.size());
    request.setContentType("text/html");
	header = generateHeader(request.getResponse());
	request.sendAll(header, std::strlen(header));
    request.sendAll(page);
    delete[] header;
}
