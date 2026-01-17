#include <cerrno>
#include <sys/stat.h>
#include "webserv.h"

/*
 * This function is resolving path
 * If path is directory, function should add index file 
 *      ex. /var/www/html/folder/ + index.html or index.php
 * If file not exists or server doesn't have access return error
 * 0 - success
 * 1 - not found
 * 2 - forbidden
*/
int Server::resolve_path(std::string& path, LocationConfig& location) {
    struct stat st;
    std::string tmp;

    // Check existence / access
    if (stat(path.c_str(), &st) != 0) {
        if (errno == ENOENT)
            return 1;
        return 2;
    }

    // If directory
    if (S_ISDIR(st.st_mode)) {
        if (path[path.size() - 1] != '/')
            return 2;

        for (std::vector<std::string>::iterator it = location.index.begin();
             it != location.index.end();
             ++it) {

            tmp = path + *it;

            if (stat(tmp.c_str(), &st) != 0) {
                if (errno == ENOENT)
                    continue;
                return 2;
            }

            if (!S_ISREG(st.st_mode))
                continue;

            path = tmp;
            return 0;
        }

        // Directory exists but no index file
        return 1;
    }

    // If regular file
    if (!S_ISREG(st.st_mode))
        return 1;

    return 0;
}

/*
 * This function resolve uri location
 * Server can have multiple location (check nginx config files)
 * Location have path / or /images
 * This function compares uri with each location path and returns closest one
 * If nothing found, returns first one
*/
LocationConfig& Server::resolve_location(
    const std::string& uri,
    std::string& fs_path
) {
    LocationConfig* best = NULL;
    size_t best_len = 0;

    for (std::vector<LocationConfig>::iterator it = config_.locations.begin();
         it != config_.locations.end();
         ++it) {

        const std::string& loc_path = it->path;

        if (uri.compare(0, loc_path.size(), loc_path) == 0) {
            if (loc_path.size() > best_len) {
                best = &(*it);
                best_len = loc_path.size();
            }
        }
    }

    // Fallback (should always exist)
    if (!best)
        best = &config_.locations[0];

    // Build filesystem path
    fs_path = best->root + "/" + uri.substr(best_len);

    return *best;
}
