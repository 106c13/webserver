#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
#include "webserv.h"
#include "defines.h"

int Server::resolvePath(std::string& path, LocationConfig& location) {
    struct stat st;
    std::string tmp;

    if (stat(path.c_str(), &st) != 0) {
        if (errno == ENOENT)
            return NOT_FOUND;
        return FORBIDDEN;
    }

    if (S_ISDIR(st.st_mode)) {
        if (access(path.c_str(), R_OK | X_OK) != 0) {
            return FORBIDDEN;
        }

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
                return FORBIDDEN;
            }

            if (!S_ISREG(st.st_mode)) {
                continue;
            }

            path = tmp;
            return OK;
        }
        return DIRECTORY_NO_INDEX;
    }

    if (!S_ISREG(st.st_mode)) {
        return NOT_FOUND;
    }

    return OK;
}

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
		if (config_.locations.empty()) {
			throw std::runtime_error("No locations configured");
        }
		best = &config_.locations.front();
	}

    fs_path = best->root + "/" + fs_path.substr(best_len);

    return *best;
}
