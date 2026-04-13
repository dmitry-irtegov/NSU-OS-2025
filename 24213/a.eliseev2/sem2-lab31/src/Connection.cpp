#include "Connection.h"
#include "Http.h"
#include "ResponseCache.h"
#include "unistd.h"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <system_error>

namespace proxy {

constexpr size_t READ_BUF_SIZE = 1024;

ClientConnection::ClientConnection(int socketFd)
    : socketFd(socketFd), request(std::make_shared<MessageBuffer>()),
      responseReadPos(0) {
}

ClientConnection::~ClientConnection() {
    if (response) {
        response->unsubscribe(socketFd);
    }
    ::close(socketFd);
}

bool ClientConnection::serviceRead(ResponseCache &cache,
                                   ConnectionManager &connectionManager) {
    MessageBuffer::Writer writer = request->write();
    char *ptr = writer.reserve(READ_BUF_SIZE);
    ssize_t count = ::read(socketFd, ptr, READ_BUF_SIZE);
    if (count < 0) {
        throw std::system_error(errno, std::generic_category(), "Read error");
    }
    if (count > 0) {
        writer.write(count);
    } else {
        request->end(connectionManager);
        return true;
    }

    if (response) {
        writer.commit(connectionManager);
    } else {
        // We've not made a request yet.
        // Try to parse the request line and make the request.
        http::RequestLine requestLine;
        const char *ptr = writer.data();
        http::RequestLine::ParseInfo parseInfo;

        if (http::RequestLine::parse(&ptr, ptr + writer.actualLength(),
                                     requestLine, parseInfo)) {
            std::cerr << "Request: "
                      << std::string(parseInfo.methodStart,
                                     parseInfo.versionEnd)
                      << std::endl;

            char *httpVersion = const_cast<char *>(parseInfo.versionStart);
            std::memcpy(httpVersion, "HTTP/1.0", 8);
            writer.removeRange(parseInfo.uri.start, parseInfo.uri.pathStart);

            writer.commit(connectionManager);
            if (requestLine.method == http::RequestMethod::Get) {
                response = cache.getEntry(requestLine.uri);
            }
            if (response) {
                std::cerr << "Sending cached response." << std::endl;
            } else {
                std::cerr << "Forwarding to server." << std::endl;
                response = connectionManager.makeRequest(requestLine, request,
                                                         socketFd);
            }
            response->subscribe(socketFd);
        }
    }
    return false;
}

ServiceResult ClientConnection::service(SocketEvents events,
                                        ResponseCache &cache,
                                        ConnectionManager &connectionManager) {
    try {
        if ((events & SocketEvents::Error) != SocketEvents::None) {
            // We've got an error. Terminate the connection.
            throw std::runtime_error("Socket error");
        }
        if (events == SocketEvents::None) {
            // We're being initialized. Wait for read.
            return ServiceResult::WaitRead;
        }

        if ((events & SocketEvents::Read) != SocketEvents::None) {
            if (serviceRead(cache, connectionManager)) {
                // Client has closed the connection.
                return ServiceResult::Disconnect;
            }
        }

        if (!response) {
            return ServiceResult::WaitRead;
        }

        MessageBuffer::Reader reader = response->read(responseReadPos);
        if (reader.length() > 0) {
            if ((events & SocketEvents::Write) != SocketEvents::None) {
                ssize_t count =
                    ::write(socketFd, reader.data(), reader.length());
                if (count < 0) {
                    throw std::system_error(errno, std::generic_category(),
                                            "Write error");
                }
                reader.advance(count);
            }
            return ServiceResult::WaitRead | ServiceResult::WaitWrite;
        } else if (reader.isEnd()) {
            // We've sent the entire response. Disconnect.
            return ServiceResult::Disconnect;
        } else {
            // We don't have any new response data. Wait.
            return ServiceResult::WaitRead;
        }
    } catch (std::runtime_error &re) {
        request->end(connectionManager);
        throw;
    }
}

ServerConnection::ServerConnection(int socketFd,
                                   const http::RequestLine &requestLine,
                                   std::shared_ptr<MessageBuffer> request,
                                   std::shared_ptr<MessageBuffer> response)
    : socketFd(socketFd), requestLine(requestLine), request(std::move(request)),
      response(std::move(response)), requestReadPos(0), cached(false),
      statusParsed(false) {
    this->request->subscribe(socketFd);
}

ServerConnection::~ServerConnection() {
    request->unsubscribe(socketFd);
    ::close(socketFd);
}

bool ServerConnection::serviceRead(ResponseCache &cache,
                                   ConnectionManager &connectionManager) {
    MessageBuffer::Writer writer = response->write();
    char *ptr = writer.reserve(READ_BUF_SIZE);
    ssize_t count = ::read(socketFd, ptr, READ_BUF_SIZE);
    if (count < 0) {
        throw std::system_error(errno, std::generic_category(), "Read error");
    }
    if (count > 0) {
        writer.write(count);
    } else {
        response->end(connectionManager);
        return true;
    }

    if (statusParsed) {
        writer.commit(connectionManager);
    } else {
        // We've not parsed the status line to determine what to do with
        // the response. Try to parse it.
        http::StatusLine line;
        http::StatusLine::ParseInfo parseInfo;
        const char *ptr = writer.data();

        if (http::StatusLine::parse(&ptr, ptr + writer.actualLength(), line,
                                    parseInfo)) {
            std::cerr << "Response: "
                      << std::string(parseInfo.versionStart,
                                     parseInfo.reasonEnd)
                      << std::endl;
            char *httpVersion = const_cast<char *>(parseInfo.versionStart);
            std::memcpy(httpVersion, "HTTP/1.0", 8);

            statusParsed = true;
            writer.commit(connectionManager);
            if (requestLine.method == http::RequestMethod::Get &&
                line.code == http::ResponseCode::Ok) {
                cached = cache.addEntry(requestLine.uri, response);
            }
        }
    }
    return false;
}

ServiceResult ServerConnection::service(SocketEvents events,
                                        ResponseCache &cache,
                                        ConnectionManager &connectionManager) {
    try {
        if ((events & SocketEvents::Error) != SocketEvents::None) {
            // We've got an error. Terminate the connection.
            throw std::runtime_error("Socket error");
        }

        if ((events & SocketEvents::Read) != SocketEvents::None) {
            if (serviceRead(cache, connectionManager)) {
                // Server has closed the connection.
                return ServiceResult::Disconnect;
            }
        }

        MessageBuffer::Reader reader = request->read(requestReadPos);
        if (reader.length() > 0) {
            if ((events & SocketEvents::Write) != SocketEvents::None) {
                ssize_t count =
                    ::write(socketFd, reader.data(), reader.length());
                if (count < 0) {
                    throw std::system_error(errno, std::generic_category(),
                                            "Write error");
                }
                reader.advance(count);
            }
            return ServiceResult::WaitRead | ServiceResult::WaitWrite;
        } else {
            // We don't have any new request data. Wait.
            return ServiceResult::WaitRead;
        }
    } catch (std::runtime_error &rte) {
        // Remove the cached entry if we get an error.
        if (cached) {
            cache.removeEntry(requestLine.uri);
        }
        response->end(connectionManager);
        throw;
    }
}
} // namespace proxy
