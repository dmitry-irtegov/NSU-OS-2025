#pragma once

#include "Http.h"
#include "MessageBuffer.h"
#include <memory>
#include <pthread.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace proxy {

class ResponseCache {
  public:
    ResponseCache();
    ResponseCache(const ResponseCache &) = delete;
    ResponseCache &operator=(const ResponseCache &) = delete;
    ResponseCache(ResponseCache &&) = delete;
    ResponseCache &operator=(ResponseCache &&) = delete;

    std::shared_ptr<MessageBuffer> getEntry(const http::Uri &uri);
    bool addEntry(const http::Uri &uri, std::shared_ptr<MessageBuffer> buffer);
    void removeEntry(const http::Uri &uri);

  private:
    struct Response {
        std::shared_ptr<MessageBuffer> buffer;
    };

    std::unordered_map<std::string, Response> map;
    pthread_mutex_t lock;
};

} // namespace proxy
