#include "ErrorResponse.h"
#include <cstring>

namespace proxy {

static const char BAD_REQUEST[] =
    "HTTP/1.0 400 Bad Request\r\n\r\n400 Bad request\r\n";
static const char NO_SERVER[] =
    "HTTP/1.0 500 No response from server\r\n\r\n500 No response from server\r\n";

std::shared_ptr<MessageBuffer> makeBadRequest(ConnectionManager &mgr) {
    std::shared_ptr<MessageBuffer> response = std::make_shared<MessageBuffer>();
    auto respWriter = response->write();
    respWriter.reserve(sizeof(BAD_REQUEST));
    std::memcpy(respWriter.reserve(sizeof(BAD_REQUEST)), BAD_REQUEST,
                sizeof(BAD_REQUEST));
    respWriter.write(sizeof(BAD_REQUEST) - 1);
    respWriter.commit(mgr);
    response->end(mgr);
    return response;
}

std::shared_ptr<MessageBuffer> makeNoServer(ConnectionManager &mgr) {
    std::shared_ptr<MessageBuffer> response = std::make_shared<MessageBuffer>();
    auto respWriter = response->write();
    respWriter.reserve(sizeof(NO_SERVER));
    std::memcpy(respWriter.reserve(sizeof(NO_SERVER)), NO_SERVER,
                sizeof(NO_SERVER));
    respWriter.write(sizeof(NO_SERVER) - 1);
    respWriter.commit(mgr);
    response->end(mgr);
    return response;
}

} // namespace proxy
