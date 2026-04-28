#include "Proxy.h"
#include "Connection.h"
#include "ErrorResponse.h"
#include "Http.h"
#include "MessageBuffer.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <stdexcept>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

namespace proxy {

constexpr int CONN_QUEUE_SIZE = 10;

Proxy::Proxy(uint16_t listenPort, std::string defaultHost, uint16_t defaultPort)
    : defaultHost(defaultHost), defaultPort(defaultPort), messageReadPos(0),
      pollLock(PTHREAD_MUTEX_INITIALIZER),
      connectionLock(PTHREAD_MUTEX_INITIALIZER) {
    int pipeFds[2];
    if (::pipe(pipeFds)) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not open control pipe");
    }

    controlReadFd = pipeFds[0];
    controlWriteFd = pipeFds[1];

    try {
        int sockFd = ::socket(PF_INET, SOCK_STREAM, 0);
        if (sockFd == -1) {
            throw std::system_error(errno, std::generic_category(),
                                    "Could not open listening socket");
        }

        try {
            int reuseAddr = 1;
            if (::setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr,
                             sizeof(reuseAddr))) {
                throw std::system_error(errno, std::generic_category(),
                                        "Could not set SO_REUSEADDR");
            }

            sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = ::htons(listenPort);

            if (bind(sockFd, (struct sockaddr *)&addr, sizeof(addr))) {
                throw std::system_error(errno, std::generic_category(),
                                        "Could not bind listening socket");
            }
            if (listen(sockFd, CONN_QUEUE_SIZE)) {
                throw std::system_error(errno, std::generic_category(),
                                        "Could not listen on socket");
            }
            pollFds.push_back({
                .fd = sockFd,
                .events = POLLIN,
                .revents = 0,
            });
            pollFds.push_back({
                .fd = controlReadFd,
                .events = POLLIN,
                .revents = 0,
            });
        } catch (std::runtime_error &re) {
            ::close(sockFd);
        }
    } catch (std::runtime_error &re) {
        ::close(controlReadFd);
        ::close(controlWriteFd);
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

    pthread_mutex_lock(&connectionLock);
    connections[serverFd] = {
        .connection = serverConnection,
        .fd = serverFd,
        .isConnected = !pending,
        .isRunning = false,
    };
    pthread_mutex_unlock(&connectionLock);

    sendControlMessage({
        .fd = serverFd,
        .events = pending ? SocketEvents::Write : SocketEvents::ReadWrite,
        .action = ControlMessage::Action::Add,
    });

    return response;
}

void Proxy::notifyWrite(int fd) {
    sendControlMessage({
        .fd = fd,
        .events = SocketEvents::Write,
        .action = ControlMessage::Action::Update,
    });
}

void Proxy::acceptNewConnections() {
    pollfd &serverPollFd = pollFds[0];
    if (!(serverPollFd.revents & POLLIN)) {
        return;
    }
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    int clientFd =
        ::accept4(serverPollFd.fd, (sockaddr *)&addr, &addr_len, SOCK_NONBLOCK);
    if (clientFd == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not accept client");
    }
    auto clientConnection = std::make_shared<ClientConnection>(clientFd);

    pthread_mutex_lock(&connectionLock);
    connections[clientFd] = {
        .connection = clientConnection,
        .fd = clientFd,
        .isConnected = true,
        .isRunning = false,
    };
    pthread_mutex_unlock(&connectionLock);

    auto it = fdIndexMap.find(clientFd);
    assert(it == fdIndexMap.end() && "Client fd still in use");

    pollFds.push_back({
        .fd = clientFd,
        .events = POLLRDNORM,
        .revents = 0,
    });
    fdIndexMap[clientFd] = pollFds.size() - 1;
}

void Proxy::sendControlMessage(ControlMessage message) {
    if (::write(controlWriteFd, reinterpret_cast<const char *>(&message),
                sizeof(ControlMessage)) != sizeof(ControlMessage)) {
        throw std::system_error(errno, std::generic_category(),
                                "Could not write to control pipe");
    }
}

bool Proxy::readControlMessage(ControlMessage &message) {
    pollfd &controlPollFd = pollFds[1];
    while (true) {
        if (::poll(&controlPollFd, 1, 0) == -1) {
            throw std::system_error(errno, std::generic_category(),
                                    "Pipe poll failed");
        }
        if (!(controlPollFd.revents & POLLIN)) {
            return false;
        }

        ssize_t count = ::read(controlPollFd.fd, messageBuffer,
                               sizeof(messageBuffer) - messageReadPos);
        if (count < 0) {
            throw std::system_error(errno, std::generic_category(),
                                    "Could not read from control pipe");
        }
        messageReadPos += count;
        if (messageReadPos != sizeof(messageBuffer)) {
            continue;
        }
        messageReadPos = 0;
        std::memcpy(&message, messageBuffer, sizeof(ControlMessage));
        return true;
    }
}

void Proxy::handleControlMessages() {
    ControlMessage message;
    while (readControlMessage(message)) {
        int fd = message.fd;
        auto it = fdIndexMap.find(fd);

        switch (message.action) {
        case ControlMessage::Action::Add: {
            assert(it == fdIndexMap.end() && "Tried to add a pollfd twice");
            pollFds.push_back({
                .fd = fd,
                .events = static_cast<decltype(pollfd::events)>(message.events),
                .revents = 0,
            });
            fdIndexMap[message.fd] = pollFds.size() - 1;
        } break;
        case ControlMessage::Action::Close: {
            assert(it != fdIndexMap.end() &&
                   "Tried to close a non-existent fd");
            size_t index = it->second;
            pollFds.erase(pollFds.begin() + index);
            fdIndexMap.erase(fd);
            for (auto &entry : fdIndexMap) {
                if (entry.second > index) {
                    entry.second--;
                }
            }
            eventQueue.erase(
                std::remove_if(eventQueue.begin(), eventQueue.end(),
                               [fd](pollfd pollfd) { return pollfd.fd == fd; }),
                eventQueue.end());
            ::close(fd);
        } break;
        case ControlMessage::Action::Update: {
            if (it == fdIndexMap.end()) {
                // Stale fd, ignore
                break;
            }
            pollfd &pollFd = pollFds[it->second];
            if (pollFd.revents) {
                pollFd.revents = 0;
            }
            pollFd.events |=
                static_cast<decltype(pollfd::events)>(message.events);
        } break;
        }
    }
}

void Proxy::disconnect(ConnectionInfo &connection) {
    int fd = connection.fd;

    pthread_mutex_lock(&connectionLock);
    connections.erase(fd);
    pthread_mutex_unlock(&connectionLock);

    sendControlMessage({
        .fd = fd,
        .action = ControlMessage::Action::Close,
    });
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
        return;
    }

    pthread_mutex_lock(&connectionLock);
    auto it = connections.find(connection.fd);
    if (it != connections.end()) {
        it->second.isConnected = true;
        it->second.isRunning = false;
    }
    pthread_mutex_unlock(&connectionLock);

    sendControlMessage({
        .fd = connection.fd,
        .events = SocketEvents::ReadWrite,
        .action = ControlMessage::Action::Update,
    });
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
        return;
    }

    pthread_mutex_lock(&connectionLock);
    auto it = connections.find(connection.fd);
    if (it != connections.end()) {
        it->second.isRunning = false;
    }
    pthread_mutex_unlock(&connectionLock);

    sendControlMessage({
        .fd = connection.fd,
        .events = static_cast<SocketEvents>(
            static_cast<decltype(pollfd::events)>(result) >> 1),
        .action = ControlMessage::Action::Update,
    });
}

void Proxy::service() {
    pthread_mutex_lock(&pollLock);

    for (auto event = eventQueue.begin(); event != eventQueue.end(); event++) {
        pthread_mutex_lock(&connectionLock);
        auto it = connections.find(event->fd);
        if (it == connections.end() || it->second.isRunning) {
            pthread_mutex_unlock(&connectionLock);
            continue;
        }
        SocketEvents events = static_cast<SocketEvents>(event->revents);
        eventQueue.erase(event);
        pthread_mutex_unlock(&pollLock);

        ConnectionInfo connection = it->second;
        it->second.isConnected = true;
        it->second.isRunning = true;
        pthread_mutex_unlock(&connectionLock);

        if (connection.isConnected) {
            serviceConnected(connection, events);
        } else {
            servicePending(connection, events);
        }
        return;
    }

    if (::poll(pollFds.data(), pollFds.size(), -1) == -1) {
        throw std::system_error(errno, std::generic_category(), "Poll failed");
    }
    handleControlMessages();
    for (size_t i = 2; i < pollFds.size(); i++) {
        pollfd &pollFd = pollFds[i];
        assert(!(pollFd.events & POLLNVAL) && "Pollfds got messed up");
        if (pollFd.revents) {
            eventQueue.push_back(pollFd);
            pollFd.events = 0;
            pollFd.revents = 0;
        }
    }
    acceptNewConnections();

    pthread_mutex_unlock(&pollLock);
}

} // namespace proxy
