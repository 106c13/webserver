#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include "webserv.h"

std::string getTime() {
    char buffer[9];
    std::time_t now = std::time(NULL);
    std::tm *t = std::localtime(&now);

    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", t);
    return std::string(buffer);
}

std::string generateRandomName(const std::string& prefix) {
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string name = prefix;
    
    for (int i = 0; i < 10; ++i) {
        name += charset[rand() % (sizeof(charset) - 1)];
    }
    return name;
}

void log(int type, const std::string& msg) {

    std::string time = getTime();

    if (type == INFO) {
        std::cout << "[" << time << "] "
                  << COLOR_BLUE
                  << "[INFO] "
                  << msg
                  << COLOR_RESET
                  << "\n";

    } else if (type == ERROR) {
        std::cerr << "[" << time << "] "
                  << COLOR_RED
                  << "[ERROR] "
                  << msg
                  << COLOR_RESET
                  << "\n";

    } else if (type == WARNING) {
        std::cout << "[" << time << "] "
                  << COLOR_YELLOW
                  << "[WARNING] "
                  << msg
                  << COLOR_RESET
                  << "\n";
    } else if (type == DEBUG) {
        std::cout << "[" << time << "] "
                  << COLOR_GREEN
                  << "[DEBUG] "
                  << msg
                  << COLOR_RESET
                  << "\n";
	}
}

ssize_t getFileSize(const std::string& path) {
	struct stat st;

	if (stat(path.c_str(), &st) < 0) {
		return -1;
	}

	return st.st_size;
}

static int detectType(const std::string& name, bool is_dir) {
    if (is_dir) {
        return 1;
	}

    size_t dot = name.rfind('.');
    if (dot == std::string::npos) {
        return 3;
	}

    std::string ext = name.substr(dot + 1);

    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "gif") {
        return 2;
	}

    return 3;
}

std::vector<DirEntry> listDirectory(const std::string& path) {
    std::vector<DirEntry> entries;
	DirEntry d;
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        return entries;
	}

    struct dirent* ent;
	while ((ent = readdir(dir)) != NULL) {
		d.name = ent->d_name;

		if (d.name == ".")
			continue;

		std::string full = path + "/" + d.name;

		struct stat st;
		if (stat(full.c_str(), &st) == 0) {
			d.is_dir = S_ISDIR(st.st_mode);
			d.size   = st.st_size;
		} else {
			d.is_dir = false;
			d.size   = 0;
		}
		d.type = detectType(full, d.is_dir);

		entries.push_back(d);
	}

    closedir(dir);
    return entries;
}
