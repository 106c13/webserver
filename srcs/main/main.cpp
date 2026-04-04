#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include "webserv.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file>\n";
        return 1;
    }

    try {
        ConfigParser parser;
        parser.parse(argv[1]);
        const Config config = parser.getConfig();

        std::vector<pid_t> children;

        for (size_t i = 0; i < config.servers.size(); ++i) {
            pid_t pid = fork();

            if (pid < 0) {
                log(ERROR, "Can't start server process");
                return 1;
            }

            if (pid == 0) {
                Server server(config.servers[i]);
                server.loop();
                exit(0);
            } else {
                children.push_back(pid);
            }
        }

        for (size_t i = 0; i < children.size(); ++i) {
            waitpid(children[i], NULL, 0);
        }

    } catch (const std::exception& e) {
        log(ERROR, e.what());
        return 1;
    }

    return 0;
}
