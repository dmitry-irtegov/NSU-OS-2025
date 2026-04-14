#pragma once

#include "Connection.h"
#include "MessageBuffer.h"
#include <memory>

namespace proxy {
    std::shared_ptr<MessageBuffer> makeBadRequest(ConnectionManager &mgr);
    std::shared_ptr<MessageBuffer> makeNoServer(ConnectionManager &mgr);
}//
