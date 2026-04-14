#include "Proxy.h"
#include <cstdint>
#include <iostream>
#include <unistd.h>

using namespace proxy;

struct ProxyArgs {
    uint16_t localPort = 8080;
    std::string remoteHost = "localhost";
    uint16_t remotePort = 80;
    bool help = false;

    static bool parse(int argc, char *argv[], ProxyArgs &args) {
        args = ProxyArgs();
        char ch;
        while ((ch = ::getopt(argc, argv, "l:r:h")) != -1) {
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
    stream << "Usage: " << progamName << " [-l PORT] [-r HOST[:PORT]] [-h]\n"
           << " -l PORT        : The port to listen on. "
              "Default: 8080\n"
           << " -r HOST[:PORT] : The host/port to forward the request "
              "to when a full URI is not given. Default: localhost:80\n"
           << " -h             : Display this help message.\n";
}

int main(int argc, char *argv[]) {
    ProxyArgs args;
    if (ProxyArgs::parse(argc, argv, args)) {
        printHelp(std::cerr, argv[0]);
        return 1;
    }
    if (args.help) {
        printHelp(std::cout, argv[0]);
        return 0;
    }
    try {
        Proxy proxy = Proxy(args.localPort, args.remoteHost, args.remotePort);
        while (true) {
            proxy.service();
        }
    } catch (std::bad_alloc &) {
        std::cerr << "Out of memory!" << std::endl;
        return 1;
    } catch (std::exception &exception) {
        std::cerr << "Error: " << exception.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error." << std::endl;
        return 1;
    }
}
