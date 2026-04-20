#include "Proxy.h"
#include "Connection.h"
#include "ErrorResponse.h"
#include "Http.h"
#include "MessageBuffer.h"
#include <arpa/inet.h>
#include <iostream>
#include <netdb.h>
#include <poll.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace proxy {

constexpr int CONN_QUEUE_SIZE = 10;

Proxy::Proxy(uint16_t listenPort, std::string defaultHost, uint16_t defaultPort)
    : defaultHost(defaultHost), defaultPort(defaultPort) {
    int sockFd = ::socket(PF_INET, SOCK_STREAM, 0);
    if (sockFd == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not open listening socket");
    }

    int reuseAddr = 1;
    if (::setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr,
                     sizeof(reuseAddr))) {
        int error = errno;
        ::close(sockFd);
        throw std::system_error(error, std::generic_category(),
                                "Could not set SO_REUSEADDR");
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = ::htons(listenPort);

    if (bind(sockFd, (struct sockaddr *)&addr, sizeof(addr))) {
        int error = errno;
        ::close(sockFd);
        throw std::system_error(error, std::generic_category(),
                                "Could not bind listening socket");
    }
    if (listen(sockFd, CONN_QUEUE_SIZE)) {
        int error = errno;
        ::close(sockFd);
        throw std::system_error(error, std::generic_category(),
                                "Could not listen on socket");
    }
    pollFds.push_back({
        .fd = sockFd,
        .events = POLLIN,
    });
    connections.push_back({});
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

    int sockFd = ::socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (sockFd == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not open target socket");
    }

    pending = false;
    if (::connect(sockFd, (sockaddr *)&sockAddr, addrLen)) {
        if (errno == EINPROGRESS) {
            pending = true;
        } else {
            int error = errno;
            ::close(sockFd);
            throw std::system_error(error, std::generic_category(),
                                    "Could not connect socket");
        }
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

    connections.push_back({
        .connection = serverConnection,
        .fd = serverFd,
        .isConnected = !pending,
    });
    pollFds.push_back({
        .fd = serverFd,
        .events = static_cast<short>(pending ? POLLOUT : POLLRDNORM | POLLOUT),
    });
    fdIndexMap[serverFd] = pollFds.size() - 1;

    return response;
}

void Proxy::notifyWrite(int fd) {
    auto it = fdIndexMap.find(fd);
    if (it == fdIndexMap.end()) {
        return;
    }
    pollFds[it->second].events |= POLLOUT;
}

void Proxy::acceptNewConnections() {
    pollfd &serverPollFd = pollFds[0];
    if (serverPollFd.revents & POLLIN) {
        sockaddr_in addr;
        socklen_t addr_len = sizeof(addr);
        int clientFd = accept4(serverPollFd.fd, (sockaddr *)&addr, &addr_len,
                               SOCK_NONBLOCK);
        if (clientFd == -1) {
            throw std::system_error(errno, std::generic_category(),
                                    "Could not accept client");
        }
        auto clientConnection = std::make_shared<ClientConnection>(clientFd);

        connections.push_back({
            .connection = clientConnection,
            .fd = clientFd,
            .isConnected = true,
        });
        pollFds.push_back({
            .fd = clientFd,
            .events = POLLRDNORM,
        });
        fdIndexMap[clientFd] = pollFds.size() - 1;
    }
}

void Proxy::disconnect(ConnectionInfo &connection) {
    int fd = connection.fd;
    size_t index = fdIndexMap[fd];
    connections.erase(connections.begin() + index);
    pollFds.erase(pollFds.begin() + index);
    fdIndexMap.erase(fd);
    for (auto &entry : fdIndexMap) {
        if (entry.second > index) {
            entry.second--;
        }
    }
    ::close(fd);
}

void Proxy::servicePending(ConnectionInfo &connection, SocketEvents events) {
    int error;
    socklen_t error_size = sizeof(error);
    if (::getsockopt(connection.fd, SOL_SOCKET, SO_ERROR, &error,
                     &error_size)) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not get connect result");
    }
    if (error) {
        serviceConnected(connection, SocketEvents::Error);
    } else {
        connection.isConnected = true;
        size_t index = fdIndexMap[connection.fd];
        pollFds[index].events = POLLRDNORM | POLLOUT;
    }
}

void Proxy::serviceConnected(ConnectionInfo &connection, SocketEvents events) {
    ServiceResult result;
    try {
        result = connection.connection->service(events, cache, *this);
    } catch (std::runtime_error &re) {
        result = ServiceResult::Disconnect;
        std::cerr << "Error while servicing connection: " << re.what()
                  << std::endl;
    }
    if ((result & ServiceResult::Disconnect) != ServiceResult::None) {
        disconnect(connection);
    } else {
        size_t index = fdIndexMap[connection.fd];
        pollFds[index].events |=
            static_cast<decltype(pollfd::events)>(result) >> 1;
    }
}

void Proxy::service() {
    acceptNewConnections();

    SocketEvents events;
    ConnectionInfo *connection = nullptr;
    for (size_t i = 1; i < pollFds.size(); i++) {
        pollfd &pollFd = pollFds[i];
        ConnectionInfo &conn = connections[i];
        if (pollFd.revents) {
            events = static_cast<SocketEvents>(pollFd.revents);
            pollFd.events = 0;
            pollFd.revents = 0;
            connection = &conn;
            break;
        }
    }

    if (!connection) {
        if (::poll(pollFds.data(), pollFds.size(), -1) == -1) {
            throw std::system_error(errno, std::generic_category(),
                                    "Poll failed");
        }
        return;
    }

    if (connection->isConnected) {
        serviceConnected(*connection, events);
    } else {
        servicePending(*connection, events);
    }
}

} // namespace proxy
