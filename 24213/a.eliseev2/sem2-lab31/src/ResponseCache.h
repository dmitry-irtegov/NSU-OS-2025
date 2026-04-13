#pragma once

#include "Http.h"
#include "MessageBuffer.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace proxy {

class ResponseCache {
  public:
    ResponseCache();

    std::shared_ptr<MessageBuffer> getEntry(const http::Uri &uri);
    bool addEntry(const http::Uri &uri, std::shared_ptr<MessageBuffer> buffer);
    void removeEntry(const http::Uri &uri);

  private:
    struct Response {
        std::shared_ptr<MessageBuffer> buffer;
    };

    std::unordered_map<std::string, Response> map;
};

} // namespace proxy
