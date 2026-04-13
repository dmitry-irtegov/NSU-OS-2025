#pragma once

#include "Http.h"
#include "MessageBuffer.h"
#include "ResponseCache.h"
#include <memory>
#include <poll.h>
#include <string>

namespace proxy {

enum class SocketEvents {
    None = 0,
    Read = POLLRDNORM,
    Write = POLLOUT,
    Error = POLLERR | POLLHUP,
};

constexpr SocketEvents operator&(SocketEvents lhs, SocketEvents rhs) {
    return static_cast<SocketEvents>(static_cast<int>(lhs) &
                                     static_cast<int>(rhs));
}

constexpr SocketEvents operator|(SocketEvents lhs, SocketEvents rhs) {
    return static_cast<SocketEvents>(static_cast<int>(lhs) |
                                     static_cast<int>(rhs));
}

enum class ServiceResult {
    None = 0,
    Disconnect = 1,
    WaitRead = POLLRDNORM << 1,
    WaitWrite = POLLOUT << 1,
};

constexpr ServiceResult operator&(ServiceResult lhs, ServiceResult rhs) {
    return static_cast<ServiceResult>(static_cast<int>(lhs) &
                                      static_cast<int>(rhs));
}

constexpr ServiceResult operator|(ServiceResult lhs, ServiceResult rhs) {
    return static_cast<ServiceResult>(static_cast<int>(lhs) |
                                      static_cast<int>(rhs));
}

class ConnectionManager : public MessageNotifier {
  public:
    virtual std::shared_ptr<MessageBuffer>
    makeRequest(const http::RequestLine &requestLine,
                std::shared_ptr<MessageBuffer> request, int clientFd) = 0;
};

class Connection {
  public:
    virtual ServiceResult service(SocketEvents events, ResponseCache &cache,
                                  ConnectionManager &connectionManager) = 0;
};

class ClientConnection final : public Connection {
  public:
    ClientConnection(int fd);
    ~ClientConnection();

    ServiceResult service(SocketEvents events, ResponseCache &cache,
                          ConnectionManager &connectionManager) override;

  private:
    bool serviceRead(ResponseCache &cache,
                     ConnectionManager &connectionManager);

    const int socketFd;
    std::shared_ptr<MessageBuffer> request;
    std::shared_ptr<MessageBuffer> response;
    size_t responseReadPos;
};

class ServerConnection final : public Connection {
  public:
    ServerConnection(int fd, const http::RequestLine &requestLine,
                     std::shared_ptr<MessageBuffer> request,
                     std::shared_ptr<MessageBuffer> response);
    ~ServerConnection();

    ServiceResult service(SocketEvents events, ResponseCache &cache,
                          ConnectionManager &connectionManager) override;

  private:
    bool serviceRead(ResponseCache &cache,
                     ConnectionManager &connectionManager);

    const int socketFd;
    http::RequestLine requestLine;
    std::shared_ptr<MessageBuffer> request;
    std::shared_ptr<MessageBuffer> response;
    size_t requestReadPos;
    bool cached;
    bool statusParsed;
};

} // namespace proxy
