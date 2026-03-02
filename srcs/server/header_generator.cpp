#include "HeaderGenerator.h"
#include "webserv.h"
#include "defines.h"
#include <map>

static std::string getStatus(int code) {
    if (code == OK) {
        return "200 OK";
    } else if (code == NO_CONTENT) {
        return "204 No Content";
    } else if (code == REDIRECT) {
        return "301 Moved Permanently";
    } else if (code == BAD_REQUEST) {
        return "400 Bad Request";
    } else if (code == FORBIDDEN) {
        return "403 Forbidden";
    } else if (code == NOT_FOUND) {
        return "404 Not Found";
    } else if (code == SERVER_ERROR) {
        return "500 Internal Server Error";
    } else {
        return "500 Internal Server Error";
    }
}

static std::string resolveContentType(const std::string& ext) {
    static std::map<std::string, std::string> mimeTypes;

    if (mimeTypes.empty()) {
        // Text types
        mimeTypes["html"] = "text/html";
        mimeTypes["htm"] = "text/html";
        mimeTypes["css"] = "text/css";
        mimeTypes["js"] = "text/javascript";
        mimeTypes["txt"] = "text/plain";
        mimeTypes["xml"] = "text/xml";
        mimeTypes["json"] = "application/json";
        mimeTypes["csv"] = "text/csv";

        // Document types
        mimeTypes["pdf"] = "application/pdf";
        mimeTypes["doc"] = "application/msword";
        mimeTypes["docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
        mimeTypes["xls"] = "application/vnd.ms-excel";
        mimeTypes["xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";

        // Image types
        mimeTypes["png"] = "image/png";
        mimeTypes["jpg"] = "image/jpeg";
        mimeTypes["jpeg"] = "image/jpeg";
        mimeTypes["gif"] = "image/gif";
        mimeTypes["svg"] = "image/svg+xml";
        mimeTypes["ico"] = "image/x-icon";
        mimeTypes["webp"] = "image/webp";
        mimeTypes["bmp"] = "image/bmp";
        mimeTypes["tiff"] = "image/tiff";
        mimeTypes["tif"] = "image/tiff";

        // Font types
        mimeTypes["woff"] = "font/woff";
        mimeTypes["woff2"] = "font/woff2";
        mimeTypes["ttf"] = "font/ttf";
        mimeTypes["otf"] = "font/otf";
        mimeTypes["eot"] = "application/vnd.ms-fontobject";

        // Audio types
        mimeTypes["mp3"] = "audio/mpeg";
        mimeTypes["wav"] = "audio/wav";
        mimeTypes["ogg"] = "audio/ogg";
        mimeTypes["m4a"] = "audio/mp4";
        mimeTypes["flac"] = "audio/flac";

        // Video types
        mimeTypes["mp4"] = "video/mp4";
        mimeTypes["webm"] = "video/webm";
        mimeTypes["avi"] = "video/x-msvideo";
        mimeTypes["mov"] = "video/quicktime";
        mimeTypes["mkv"] = "video/x-matroska";
        mimeTypes["mpeg"] = "video/mpeg";
        mimeTypes["mpg"] = "video/mpeg";

        // Archive types
        mimeTypes["zip"] = "application/zip";
        mimeTypes["gz"] = "application/gzip";
        mimeTypes["gzip"] = "application/gzip";
        mimeTypes["tar"] = "application/x-tar";
        mimeTypes["rar"] = "application/vnd.rar";
        mimeTypes["7z"] = "application/x-7z-compressed";

        // Other common types
        mimeTypes["sh"] = "application/x-sh";
        mimeTypes["jar"] = "application/java-archive";
        mimeTypes["swf"] = "application/x-shockwave-flash";
        mimeTypes["wasm"] = "application/wasm";
    }

    std::map<std::string, std::string>::const_iterator it = mimeTypes.find(ext);
    if (it != mimeTypes.end()) {
        return it->second;
    }
    return "application/octet-stream";
}

std::string generateHeader(const struct Response& res) {
    std::string header;
    std::string contentType;
    std::string ext;

    header += res.version;
    header += " ";
    header += getStatus(res.status);
    header += "\r\n";

    if (!res.contentType.empty()) {
        contentType = res.contentType;
    } else {
        size_t dot = res.path.rfind('.');
        if (dot != std::string::npos && dot + 1 < res.path.size())
            ext = res.path.substr(dot + 1);
        contentType = resolveContentType(ext);
    }

    if (!res.location.empty()) {
        header += "Location: " + res.location + "\r\n";
    }

    header += "Content-Type: " + contentType + "\r\n";
    header += "Content-Length: " + res.contentLength + "\r\n";

    if (!res.contentDisposition.empty()) {
        header += "Content-Disposition: " + res.contentDisposition + "\r\n";
    }

    if (res.cookie) {
        header += "Set-Cookie: ";
        header += res.cookie->value;
        header += "\r\n";
    }

    header += "Connection: " + res.connectionType + "\r\n";
    header += "\r\n";
	
	return header;
}
