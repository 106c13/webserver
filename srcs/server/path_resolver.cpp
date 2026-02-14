#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
#include "webserv.h"

/*
 * This function is resolving path
 * If path is directory, function should add index file 
 *      ex. /var/www/html/folder/ + index.html or index.php
 * If file not exists or server doesn't have access return error
 * Returns:
 *  0 - success
 *  1 - path exists but file not found
 *  2 - path or file not exists, like uri = /unkown/ or /unkown.html
 *  3 - forbidden
 *
*/
int Server::resolvePath(std::string& path, LocationConfig& location) {
    struct stat st;
    std::string tmp;

    // Check existence / access
    if (stat(path.c_str(), &st) != 0) {
        if (errno == ENOENT)
            return 2;  // Not found
        return 3;      // Forbidden or other error
    }

    if (S_ISDIR(st.st_mode)) {
        if (access(path.c_str(), R_OK | X_OK) != 0)
            return 3;

        for (std::vector<std::string>::iterator it = location.index.begin();
             it != location.index.end();
             ++it) {

            tmp = path;
            if (!tmp.empty() && tmp[tmp.size() - 1] != '/')
                tmp += "/";
            tmp += *it;

            if (stat(tmp.c_str(), &st) != 0) {
                if (errno == ENOENT)
                    continue;
                return 3;
            }

            if (!S_ISREG(st.st_mode))
                continue;

            if (access(tmp.c_str(), R_OK) != 0)
                return 3;

            path = tmp;
            return 0;
        }
        return 1;
    }

    if (!S_ISREG(st.st_mode))
        return 2;
    if (access(path.c_str(), R_OK) != 0)
        return 3;
    return 0;
}

/*
 * This function resolve uri location
 * Server can have multiple location (check nginx config files)
 * Location have path / or /images
 * This function compares uri with each location path and returns closest one
 * If nothing found, returns first one
*/
LocationConfig& Server::resolveLocation(std::string& fs_path) {
    LocationConfig* best = NULL;
    size_t best_len = 0;

    for (LocationList::iterator it = config_.locations.begin();
         it != config_.locations.end();
         ++it) {

        const std::string& loc_path = it->path;

        if (fs_path.compare(0, loc_path.size(), loc_path) == 0) {
            if (loc_path.size() > best_len) {
                best = &(*it);
                best_len = loc_path.size();
            }
        }
    }

    if (!best) {
		if (config_.locations.empty())
			throw std::runtime_error("No locations configured");
		best = &config_.locations.front();
	}

    fs_path = best->root + "/" + fs_path.substr(best_len);

    return *best;
}
