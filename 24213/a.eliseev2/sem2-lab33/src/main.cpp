#include "Proxy.h"
#include <cstdint>
#include <iostream>
#include <unistd.h>

using namespace proxy;

struct ProxyArgs {
    uint16_t localPort = 8080;
    std::string remoteHost = "localhost";
    uint16_t remotePort = 80;
    int numWorkers = 8;
    bool help = false;

    static bool parse(int argc, char *argv[], ProxyArgs &args) {
        args = ProxyArgs();
        char ch;
        while ((ch = ::getopt(argc, argv, "l:r:j:h")) != -1) {
            switch (ch) {
            case 'l':
                if (parsePort(optarg, args.localPort)) {
                    std::cerr << "Invalid local port." << std::endl;
                    return true;
                }
                break;
            case 'r':
                if (parseHostAndPort(optarg, args.remoteHost,
                                     args.remotePort)) {
                    std::cerr << "Invalid remote host/port." << std::endl;
                    return true;
                }
                break;
            case 'j':
                if (parseNumWorkers(optarg, args.numWorkers)) {
                    std::cerr << "Invalid number of worker threads. "
                                 "1 to 255 expected."
                              << std::endl;
                    return true;
                }
                break;
            case 'h':
                args.help = true;
                break;
            case '?':
                return true;
            }
        }
        return false;
    }

  private:
    static bool parseNumWorkers(const char *str, int &numWorkers) {
        long longNumWorkers;
        try {
            longNumWorkers = std::stol(str);
        } catch (...) {
            return true;
        }
        if (longNumWorkers < 1 ||
            longNumWorkers > static_cast<long>(UINT8_MAX)) {
            return true;
        }
        numWorkers = longNumWorkers;
        return false;
    }

    static bool parsePort(const char *str, uint16_t &port) {
        long longPort;
        try {
            longPort = std::stol(str);
        } catch (...) {
            return true;
        }
        if (longPort < 0 || longPort > static_cast<long>(UINT16_MAX)) {
            return true;
        }
        port = longPort;
        return false;
    }

    static bool parseHostAndPort(const char *str, std::string &host,
                                 uint16_t &port) {
        const char *ch = str;
        for (; *ch != '\0'; ch++) {
            if (*ch != ':') {
                continue;
            }
            host = std::string(str, ch);
            if (parsePort(ch + 1, port)) {
                return true;
            }
            return false;
        }
        host = std::string(str, ch);
        return false;
    }
};

static void printHelp(std::ostream &stream, const char *progamName) {
    stream << "Usage: " << progamName
           << " [-l PORT] [-r HOST[:PORT]] [-j JOBS] [-h]\n"
           << " -l PORT        : The port to listen on. "
              "Default: 8080\n"
           << " -r HOST[:PORT] : The host/port to forward the request "
              "to when a full URI is not given. Default: localhost:80\n"
           << " -j JOBS        : The number of worker threads. Default: 8\n"
           << " -h             : Display this help message.\n";
}

extern "C" void *workerRun(void *arg) {
    Proxy *proxy = static_cast<Proxy *>(arg);
    try {
        while (true) {
            proxy->service();
        }
    } catch (std::bad_alloc &) {
        std::cerr << "Out of memory!" << std::endl;
        _exit(1);
    } catch (std::exception &exception) {
        std::cerr << "Error: " << exception.what() << std::endl;
        exit(1);
    } catch (...) {
        std::cerr << "Unknown error." << std::endl;
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    try {
        ProxyArgs args;
        if (ProxyArgs::parse(argc, argv, args)) {
            printHelp(std::cerr, argv[0]);
            return 1;
        }
        if (args.help) {
            printHelp(std::cout, argv[0]);
            return 0;
        }

        static Proxy proxy =
            Proxy(args.localPort, args.remoteHost, args.remotePort);
        std::cerr << "Listening on port " << args.localPort << ".\n";

        for (int i = 0; i < args.numWorkers - 1; i++) {
            pthread_t thread;
            int result = pthread_create(&thread, NULL, workerRun,
                                        static_cast<void *>(&proxy));
            if (result) {
                throw std::system_error(result, std::generic_category(),
                                        "Could not open listening socket");
            }
        }
        std::cerr << "Created " << args.numWorkers << " worker threads.\n";

        while (true) {
            proxy.service();
        }
    } catch (std::bad_alloc &) {
        std::cerr << "Out of memory!" << std::endl;
        _exit(1);
    } catch (std::exception &exception) {
        std::cerr << "Error: " << exception.what() << std::endl;
        exit(1);
    } catch (...) {
        std::cerr << "Unknown error." << std::endl;
        exit(1);
    }
}
