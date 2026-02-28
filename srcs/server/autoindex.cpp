#include <dirent.h>
#include <sstream>
#include "webserv.h"

static std::string humanSize(size_t bytes)
{
    const char* units[] = {"B", "KB", "MB", "GB"};
    double size = bytes;
    int unit = 0;

    while (size >= 1024 && unit < 3) {
        size /= 1024;
        unit++;
    }

    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(1);          // 1 decimal like nginx
    ss << size << " " << units[unit];

    return ss.str();
}


void Server::generateAutoindex(Connection& conn, LocationConfig& location)
{
    Request& req = conn.req;
    Response& res = conn.res;

    std::string path = req.uri;
    if (!path.empty() && path[path.length() - 1] != '/') {
        path += '/';
    }

    std::vector<DirEntry> dirs = listDirectory(location.root + path);

    std::string page;

    page += "<!DOCTYPE html>\n";
    page += "<html>\n<head>\n";
    page += "<meta charset=\"UTF-8\">\n";
    page += "<title>Index of " + path + "</title>\n";
    page += "</head>\n<body>\n";
    page += "<h1>Index of " + path + "</h1>\n<hr>\n";

    page += "<table>\n";

    for (std::vector<DirEntry>::iterator it = dirs.begin();
         it != dirs.end();
         ++it)
    {
        std::string icon;

        if (it->type == 1)
            icon = "📁 ";
        else if (it->type == 2)
            icon = "🖼️ ";
        else
            icon = "📄 ";

        page += "<tr>";

        page += "<td>";
        page += icon;
        page += "<a style=\"margin-left:15px;\" href=\"" + path + it->name + "\">" + it->name + "</a>";
        page += "</td>";

        page += "<td style=\"text-align:right; padding-left:40px;\">";
        page += (it->is_dir ? "-" : humanSize(it->size));
        page += "</td>";

        page += "</tr>\n";
    }

    page += "</table>\n";
    page += "</body></html>";

    res.status = OK;
    res.contentType = "text/html";
    res.contentLength = toString(page.size());
    res.connectionType = "close";

    std::string header = generateHeader(res);
    conn.sendBuffer.append(header);
    conn.sendBuffer.append(page);

    modifyToWrite(conn.fd);
}
