#include "ErrorResponse.h"
#include "MessageBuffer.h"
#include <memory>

namespace proxy {
namespace error {

static const char BAD_REQUEST[] =
    "HTTP/1.0 400 Bad Request\r\n\r\nBad request\r\n";
static const char NO_SERVER[] =
    "HTTP/1.0 500 Server unavailable\r\n\r\nServer unavailable\r\n";
static const char SERVER_ERROR[] =
    "HTTP/1.0 500 Internal server error\r\n\r\nInternal server error\r\n";

void makeBadRequest(std::shared_ptr<MessageBuffer> &message,
                    ConnectionManager &connectionManager) {
    if (!message) {
        message = std::make_shared<MessageBuffer>();
    }
    auto writer = message->write();
    if (writer.actualLength() != 0) {
        return;
    }
    writer.write(BAD_REQUEST, sizeof(BAD_REQUEST) - 1);
    writer.end();
    writer.commit(connectionManager);
}

void makeNoServer(std::shared_ptr<MessageBuffer> &message,
                  ConnectionManager &connectionManager) {
    if (!message) {
        message = std::make_shared<MessageBuffer>();
    }
    auto writer = message->write();
    if (writer.actualLength() != 0) {
        return;
    }
    writer.write(NO_SERVER, sizeof(NO_SERVER) - 1);
    writer.end();
    writer.commit(connectionManager);
}

void makeServerError(std::shared_ptr<MessageBuffer> &message,
                     ConnectionManager &connectionManager) {
    if (!message) {
        message = std::make_shared<MessageBuffer>();
    }
    auto writer = message->write();
    if (writer.actualLength() != 0) {
        return;
    }
    writer.write(SERVER_ERROR, sizeof(SERVER_ERROR) - 1);
    writer.end();
    writer.commit(connectionManager);
}

} // namespace error
} // namespace proxy
