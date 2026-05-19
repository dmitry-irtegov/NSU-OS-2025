#include "Proxy.h"
#include "Connection.h"
#include "ErrorResponse.h"
#include "Http.h"
#include "MessageBuffer.h"
#include <arpa/inet.h>
#include <cassert>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace proxy {

constexpr int CONN_QUEUE_SIZE = 10;

extern "C" void onSignal(int sig) {
}

extern "C" void *threadRun(void *argument) {
    try {
        Proxy::ThreadParams *params =
            static_cast<Proxy::ThreadParams *>(argument);
        std::cerr << "Starting thread: " << params->connectionInfo.pollFd.fd
                  << std::endl;

        while (params->proxy.service(params->connectionInfo)) {
        }
        std::cerr << "Stopping thread: " << params->connectionInfo.pollFd.fd
                  << std::endl;
        delete params;
        return nullptr;
    } catch (std::bad_alloc &) {
        std::cerr << "Out of memory!" << std::endl;
        _exit(1);
    } catch (std::exception &error) {
        std::cerr << "Error: " << error.what() << std::endl;
        exit(1);
    } catch (...) {
        std::cerr << "Unknown error." << std::endl;
        exit(1);
    }
}

Proxy::Proxy(uint16_t listenPort, std::string defaultHost, uint16_t defaultPort)
    : defaultHost(defaultHost), defaultPort(defaultPort),
      lock(PTHREAD_MUTEX_INITIALIZER) {
    ::pthread_attr_init(&thread_attr);
    ::pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);

    sigset_t sigmask;
    ::sigemptyset(&sigmask);
    ::sigaddset(&sigmask, SIGUSR1);
    ::pthread_sigmask(SIG_BLOCK, &sigmask, nullptr);

    struct sigaction sa;
    sa.sa_handler = onSignal;
    sa.sa_mask = sigmask;
    sa.sa_flags = 0;
    ::sigaction(SIGUSR1, &sa, nullptr);

    listenFd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (listenFd == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not open listening socket");
    }

    try {
        int reuseAddr = 1;
        if (::setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr,
                         sizeof(reuseAddr))) {
            throw std::system_error(errno, std::generic_category(),
                                    "Could not set SO_REUSEADDR");
        }

        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = ::htons(listenPort);

        if (bind(listenFd, (struct sockaddr *)&addr, sizeof(addr))) {
            throw std::system_error(errno, std::generic_category(),
                                    "Could not bind listening socket");
        }
        if (listen(listenFd, CONN_QUEUE_SIZE)) {
            throw std::system_error(errno, std::generic_category(),
                                    "Could not listen on socket");
        }
    } catch (std::runtime_error &re) {
        ::close(listenFd);
    }
}

static sockaddr resolveAddress(const char *hostname, uint16_t port,
                               socklen_t *addrLen) {
    std::string strPort = std::to_string(port);

    addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int gaiError = ::getaddrinfo(hostname, strPort.c_str(), &hints, &result);
    if (gaiError) {
        throw std::runtime_error("Could not resolve address");
    }

    sockaddr addr = *result->ai_addr;
    *addrLen = result->ai_addrlen;
    ::freeaddrinfo(result);
    return addr;
}

int Proxy::connect(const char *hostname, uint16_t port, bool &pending) {
    socklen_t addrLen;
    sockaddr sockAddr = resolveAddress(hostname, port, &addrLen);

    int sockFd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (sockFd == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not open target socket");
    }

    if (::connect(sockFd, (sockaddr *)&sockAddr, addrLen)) {
        int error = errno;
        ::close(sockFd);
        throw std::system_error(error, std::generic_category(),
                                "Could not connect socket");
    }
    return sockFd;
}

std::shared_ptr<MessageBuffer>
Proxy::makeRequest(const http::RequestLine &line,
                   std::shared_ptr<MessageBuffer> request, int clientFd) {
    const char *host =
        line.uri.host.empty() ? defaultHost.c_str() : line.uri.host.c_str();
    uint16_t port = line.uri.port == 0 ? defaultPort : line.uri.port;

    bool pending;
    int serverFd;

    try {
        serverFd = connect(host, port, pending);
    } catch (std::runtime_error &re) {
        std::shared_ptr<MessageBuffer> response;
        error::makeNoServer(response, *this);
        return response;
    }

    std::shared_ptr<MessageBuffer> response = std::make_shared<MessageBuffer>();
    auto serverConnection =
        std::make_shared<ServerConnection>(serverFd, line, request, response);

    ThreadParams *params = new ThreadParams{
        .connectionInfo =
            {
                .connection = serverConnection,
                .pollFd =
                    {
                        .fd = serverFd,
                        .events = static_cast<short>(
                            pending ? POLLOUT : (POLLOUT | POLLRDNORM)),
                        .revents = 0,
                    },
            },
        .proxy = *this,
    };
    pthread_t thread;

    if (int error = ::pthread_create(&thread, &thread_attr, threadRun, params)) {
        std::cerr << "Could not create server thread: "
                  << std::error_code(error, std::generic_category()).message()
                  << std::endl;
        delete params;
        error::makeServerError(response, *this);
        return response;
    }

    ::pthread_mutex_lock(&lock);
    threads[serverFd] = {
        .thread = thread,
        .writeNotified = false,
    };
    ::pthread_mutex_unlock(&lock);

    return response;
}

void Proxy::notifyWrite(int fd) {
    ::pthread_mutex_lock(&lock);
    auto it = threads.find(fd);
    if (it != threads.end()) {
        it->second.writeNotified = true;
        if (int error = ::pthread_kill(it->second.thread, SIGUSR1)) {
            ::pthread_mutex_unlock(&lock);
            throw std::system_error(error, std::generic_category(),
                                    "Could not kill thread");
        }
    }
    ::pthread_mutex_unlock(&lock);
}

void Proxy::disconnect(ConnectionInfo &connection) {
    ::pthread_mutex_lock(&lock);
    auto it = threads.find(connection.pollFd.fd);
    if (it != threads.end()) {
        threads.erase(it);
    }
    ::close(connection.pollFd.fd);
    ::pthread_mutex_unlock(&lock);
}

bool Proxy::service(ConnectionInfo &connection) {
    ::pthread_mutex_lock(&lock);
    auto it = threads.find(connection.pollFd.fd);
    if (it != threads.end() && it->second.writeNotified) {
        it->second.writeNotified = false;
        connection.pollFd.events |= POLLOUT;
    }
    ::pthread_mutex_unlock(&lock);

    sigset_t sigmask;
    ::pthread_sigmask(0, nullptr, &sigmask);
    ::sigdelset(&sigmask, SIGUSR1);

    if (::ppoll(&connection.pollFd, 1, nullptr, &sigmask) == -1) {
        if (errno != EINTR) {
            throw std::system_error(errno, std::generic_category(),
                                    "Poll failed");
        }
    }

    SocketEvents events = static_cast<SocketEvents>(connection.pollFd.revents);
    connection.pollFd.revents = 0;
    connection.pollFd.events = 0;

    ServiceResult result;
    try {
        result = connection.connection->service(events, cache, *this);
    } catch (std::runtime_error &re) {
        result = ServiceResult::Disconnect;
        std::cerr << "Error while servicing connection: " << re.what()
                  << std::endl;
    }
    connection.pollFd.events |=
        static_cast<decltype(pollfd::events)>(result) >> 1;

    if ((result & ServiceResult::Disconnect) != ServiceResult::None) {
        disconnect(connection);
        return false;
    }
    return true;
}

void Proxy::acceptNewConnection() {
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int clientFd = ::accept(listenFd, (sockaddr *)&addr, &addr_len);
    if (clientFd == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not accept client");
    }
    auto clientConnection = std::make_shared<ClientConnection>(clientFd);

    ThreadParams *params = new ThreadParams{
        .connectionInfo =
            {
                .connection = clientConnection,
                .pollFd =
                    {
                        .fd = clientFd,
                        .events = POLLRDNORM,
                        .revents = 0,
                    },
            },
        .proxy = *this,
    };
    pthread_t thread;

    if (int error = ::pthread_create(&thread, &thread_attr, threadRun, params)) {
        std::cerr << "Could not create server thread: "
                  << std::error_code(error, std::generic_category()).message()
                  << std::endl;
        delete params;
        ::close(clientFd);
    }

    ::pthread_mutex_lock(&lock);
    threads[clientFd] = {
        .thread = thread,
        .writeNotified = false,
    };
    ::pthread_mutex_unlock(&lock);
}

} // namespace proxy
