#include "ResponseCache.h"

namespace proxy {

ResponseCache::ResponseCache() : map() {
}

std::shared_ptr<MessageBuffer> ResponseCache::getEntry(const http::Uri &uri) {
    auto it = map.find(uri.raw);
    if (it == map.end()) {
        return nullptr;
    }
    return it->second.buffer;
}

bool ResponseCache::addEntry(const http::Uri &uri,
                             std::shared_ptr<MessageBuffer> buffer) {
    if (map.find(uri.raw) != map.end()) {
        return false;
    }
    map[uri.raw] = {
        .buffer = std::move(buffer),
    };
    return true;
}

void ResponseCache::removeEntry(const http::Uri &uri) {
    map.erase(uri.raw);
}

} // namespace proxy
