#include <dirent.h>
#include <sstream>
#include "webserv.h"

// ── DirEntry type constants ──────────────────────────────────────────────────

static const int ENTRY_DIR   = 1;
static const int ENTRY_IMAGE = 2;

// ── internal helpers ─────────────────────────────────────────────────────────

static std::string humanSize(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    double size = bytes;
    int unit = 0;

    while (size >= 1024 && unit < 3) {
        size /= 1024;
        ++unit;
    }

    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(1);
    ss << size << " " << units[unit];
    return ss.str();
}

static const char* entryIcon(const DirEntry& entry) {
    if (entry.type == ENTRY_DIR)   return "📁 ";
    if (entry.type == ENTRY_IMAGE) return "🖼️ ";
    return "📄 ";
}

static std::string buildEntryRow(const DirEntry& entry, const std::string& path) {
    std::string size = entry.is_dir ? "-" : humanSize(entry.size);
    std::ostringstream row;
    row << "<tr>"
        << "<td>" << entryIcon(entry)
        << "<a style=\"margin-left:15px;\" href=\"" << path << entry.name << "\">"
        << entry.name << "</a></td>"
        << "<td style=\"text-align:right; padding-left:40px;\">" << size << "</td>"
        << "</tr>\n";
    return row.str();
}

static std::string buildAutoindexPage(const std::string& path,
                                      const std::vector<DirEntry>& entries) {
    std::ostringstream page;
    page << "<!DOCTYPE html>\n"
         << "<html>\n<head>\n"
         << "<meta charset=\"UTF-8\">\n"
         << "<title>Index of " << path << "</title>\n"
         << "</head>\n<body>\n"
         << "<h1>Index of " << path << "</h1>\n<hr>\n"
         << "<table>\n";

    for (std::vector<DirEntry>::const_iterator it = entries.begin();
         it != entries.end(); ++it)
        page << buildEntryRow(*it, path);

    page << "</table>\n"
         << "</body></html>";
    return page.str();
}

// ── Server method ────────────────────────────────────────────────────────────

void Server::generateAutoindex(Connection& conn, LocationConfig& location) {
    std::string path = conn.req.uri;
    if (path.empty() || path[path.size() - 1] != '/')
        path += '/';

    std::vector<DirEntry> entries = listDirectory(location.root + path);
    std::string page = buildAutoindexPage(path, entries);

    Response& res      = conn.res;
    res.status         = OK;
    res.contentType    = "text/html";
    res.contentLength  = toString(page.size());
    res.connectionType = "close";

    conn.sendBuffer.append(generateHeader(res));
    conn.sendBuffer.append(page);
    conn.state = SENDING_RESPONSE;
    modifyToWrite(conn.fd);
}
