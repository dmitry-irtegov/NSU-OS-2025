#pragma once

#include "Connection.h"
#include "Http.h"
#include "MessageBuffer.h"
#include "ResponseCache.h"
#include <cstdint>
#include <memory>
#include <unordered_map>

namespace proxy {

class Proxy : private ConnectionManager {
  public:
    Proxy(uint16_t listenPort, std::string defaultHost, uint16_t defaultPort);

    Proxy(const Proxy &) = delete;
    Proxy &operator=(const Proxy &) = delete;

    Proxy(Proxy &&) = delete;
    Proxy &operator=(Proxy &&) = delete;

    void service();

  private:
    struct ConnectionInfo {
        std::shared_ptr<Connection> connection;
        int fd;
        bool isConnected;
    };

    std::shared_ptr<MessageBuffer>
    makeRequest(const http::RequestLine &requestLine,
                std::shared_ptr<MessageBuffer> request, int clientFd) override;
    void notifyWrite(int fd) override;

    int connect(const char *hostname, uint16_t port, bool &pending);
    void acceptNewConnections();
    
    void disconnect(ConnectionInfo &connection);
    void servicePending(ConnectionInfo &connection, SocketEvents events);
    void serviceConnected(ConnectionInfo &connection, SocketEvents events);

    ResponseCache cache;
    std::vector<pollfd> pollFds;
    std::vector<ConnectionInfo> connections;
    std::unordered_map<int, size_t> fdIndexMap;

    std::string defaultHost;
    uint16_t defaultPort;
};

} // namespace proxy
