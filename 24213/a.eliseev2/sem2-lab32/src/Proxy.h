#pragma once

#include "Connection.h"
#include "Http.h"
#include "MessageBuffer.h"
#include "ResponseCache.h"
#include <cstdint>
#include <deque>
#include <memory>
#include <pthread.h>
#include <unordered_map>

namespace proxy {

class Proxy : private ConnectionManager {
  public:
    struct ConnectionInfo {
        std::shared_ptr<Connection> connection;
        pollfd pollFd;
        bool writeNotified;
    };
    struct ThreadParams {
        ConnectionInfo connectionInfo;
        Proxy &proxy;
    };

    Proxy(uint16_t listenPort, std::string defaultHost, uint16_t defaultPort);

    Proxy(const Proxy &) = delete;
    Proxy &operator=(const Proxy &) = delete;

    Proxy(Proxy &&) = delete;
    Proxy &operator=(Proxy &&) = delete;

    void acceptNewConnection();
    bool service(ConnectionInfo &connection);

  private:
    std::shared_ptr<MessageBuffer>
    makeRequest(const http::RequestLine &requestLine,
                std::shared_ptr<MessageBuffer> request, int clientFd) override;
    void notifyWrite(int fd) override;

    int connect(const char *hostname, uint16_t port, bool &pending);
    void disconnect(ConnectionInfo &connection);
    
    std::string defaultHost;
    uint16_t defaultPort;

    int listenFd;
    ResponseCache cache;
    std::unordered_map<int, pthread_t> threads;
    pthread_mutex_t lock;
};

} // namespace proxy
