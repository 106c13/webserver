#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <ctime>
#include <cstdlib>
#include "webserv.h"

int main(int argc, char **argv) {
    srand(time(NULL));
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>\n";
        return 1;
    }

    try {
        ConfigParser parser;
        parser.parse(argv[1]);
        const Config config = parser.getConfig();

        ServerManager manager(config);
        manager.run();

    } catch (const std::exception& e) {
        log(ERROR, e.what());
        return 1;
    }

    return 0;
}
