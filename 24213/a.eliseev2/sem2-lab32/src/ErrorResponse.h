#pragma once

#include "Connection.h"
#include "MessageBuffer.h"
#include <memory>

namespace proxy {
namespace error {

void makeBadRequest(std::shared_ptr<MessageBuffer> &message,
                    ConnectionManager &connectionManager);
void makeNoServer(std::shared_ptr<MessageBuffer> &message,
                  ConnectionManager &connectionManager);
void makeServerError(std::shared_ptr<MessageBuffer> &message,
                     ConnectionManager &connectionManager);

} // namespace error
} // namespace proxy
