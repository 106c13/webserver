#include <fcntl.h>
#include <sys/stat.h>
#include <iostream>
#include "webserv.h"

void log(int type, const std::string& msg) {
	if (type == INFO) {
		std::cout << COLOR_GREEN
				  << "[INFO] "
				  << msg
				  << COLOR_RESET
				  << std::endl;
	} else if (type == ERROR) {
		std::cerr << COLOR_RED
				  << "[ERROR] "
				  << msg
				  << COLOR_RESET
				  << std::endl;
	} else if (type == WARNING) {
		std::cerr << COLOR_YELLOW
				  << "[WARNING] "
				  << msg
				  << COLOR_RESET
				  << std::endl;
	}
}

bool fileExists(const std::string& path) {
    return (access(path.c_str(), F_OK) == 0);
}

bool canReadFile(const std::string& path) {
	return (access(path.c_str(), R_OK) == 0);
}

ssize_t getFileSize(const std::string& path)
{
	struct stat st;

	if (stat(path.c_str(), &st) < 0) {
		return -1;
	}

	return st.st_size;
}

std::string readFile(const std::string& filename) {
	int         fd;
	char        buffer[1024];
	ssize_t     bytes;
	std::string content;

	fd = open(filename.c_str(), O_RDONLY);

	if (fd < 0) {
		return "";
	}

	while ((bytes = read(fd, buffer, sizeof(buffer))) > 0) {
		content.append(buffer, bytes);
	}

	close(fd);
	return content;
}

static int detectType(const std::string& name, bool is_dir)
{
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
