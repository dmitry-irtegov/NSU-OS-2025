#include "ResponseCache.h"
#include <pthread.h>

namespace proxy {

ResponseCache::ResponseCache() : lock(PTHREAD_MUTEX_INITIALIZER) {
}

std::shared_ptr<MessageBuffer> ResponseCache::getEntry(const http::Uri &uri) {
    pthread_mutex_lock(&lock);
    auto it = map.find(uri.raw);
    if (it == map.end()) {
        pthread_mutex_unlock(&lock);
        return nullptr;
    }
    std::shared_ptr<MessageBuffer> buffer = it->second.buffer;
    pthread_mutex_unlock(&lock);
    return buffer;
}

bool ResponseCache::addEntry(const http::Uri &uri,
                             std::shared_ptr<MessageBuffer> buffer) {
    pthread_mutex_lock(&lock);
    if (map.find(uri.raw) != map.end()) {
        pthread_mutex_unlock(&lock);
        return false;
    }
    map[uri.raw] = {
        .buffer = std::move(buffer),
    };
    pthread_mutex_unlock(&lock);
    return true;
}

void ResponseCache::removeEntry(const http::Uri &uri) {
    pthread_mutex_lock(&lock);
    map.erase(uri.raw);
    pthread_mutex_unlock(&lock);
}

} // namespace proxy
