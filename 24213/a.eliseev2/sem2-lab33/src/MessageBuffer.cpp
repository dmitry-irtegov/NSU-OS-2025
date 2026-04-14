#include "MessageBuffer.h"
#include <cstring>
#include <pthread.h>
#include <system_error>

namespace proxy {

MessageBuffer::MessageBuffer() : data(), isEnd(false), readableSize(0) {
    pthread_mutexattr_t attr;
    if (int error = pthread_mutexattr_init(&attr)) {
        throw std::system_error(error, std::generic_category(),
                                "Could not init mutexattr");
    }
    if (int error = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE)) {
        pthread_mutexattr_destroy(&attr);
        throw std::system_error(error, std::generic_category(),
                                "Could not set mutex type");
    }
    if (int error = pthread_mutex_init(&lock, &attr)) {
        pthread_mutexattr_destroy(&attr);
        throw std::system_error(error, std::generic_category(),
                                "Could not init mutex");
    }
    pthread_mutexattr_destroy(&attr);
}

MessageBuffer::~MessageBuffer() {
    pthread_mutex_destroy(&lock);
}

MessageBuffer::Reader MessageBuffer::read(size_t &position) {
    return Reader(*this, position);
}

MessageBuffer::Writer MessageBuffer::write() {
    return Writer(*this);
}

void MessageBuffer::subscribe(int fd) {
    pthread_mutex_lock(&lock);
    subscribers.insert(fd);
    pthread_mutex_unlock(&lock);
}

void MessageBuffer::unsubscribe(int fd) {
    pthread_mutex_lock(&lock);
    subscribers.erase(fd);
    pthread_mutex_unlock(&lock);
}

void MessageBuffer::end(MessageNotifier &notifier) {
    pthread_mutex_lock(&lock);
    isEnd = true;
    for (int fd : subscribers) {
        notifier.notifyWrite(fd);
    }
    pthread_mutex_unlock(&lock);
}

MessageBuffer::Reader::Reader(MessageBuffer &buffer, size_t &position)
    : buffer(&buffer), position(&position) {
    pthread_mutex_lock(&buffer.lock);
}

MessageBuffer::Reader::~Reader() {
    pthread_mutex_unlock(&buffer->lock);
}

const char *MessageBuffer::Reader::data() {
    return buffer->data.data() + *position;
}

size_t MessageBuffer::Reader::length() {
    return buffer->readableSize - *position;
}

bool MessageBuffer::Reader::isEnd() {
    return (buffer->readableSize - *position) == 0 && buffer->isEnd;
}

void MessageBuffer::Reader::advance(size_t size) {
    *position = std::min(buffer->readableSize, *position + size);
}

MessageBuffer::Writer::Writer(MessageBuffer &buffer)
    : buffer(&buffer), reservedSize(0) {
    pthread_mutex_lock(&buffer.lock);
}

MessageBuffer::Writer::~Writer() {
    pthread_mutex_unlock(&buffer->lock);
}

char *MessageBuffer::Writer::data() {
    return buffer->data.data();
}

size_t MessageBuffer::Writer::actualLength() {
    return buffer->data.size();
}

char *MessageBuffer::Writer::reserve(size_t size) {
    size_t oldSize = buffer->data.size() - reservedSize;
    buffer->data.resize(oldSize + size);
    reservedSize = size;
    return buffer->data.data() + oldSize;
}

void MessageBuffer::Writer::write(size_t size) {
    buffer->data.resize(buffer->data.size() - reservedSize + size);
    reservedSize = 0;
}

void MessageBuffer::Writer::write(const char *data, size_t size) {
    std::memcpy(reserve(size), data, size);
    write(size);
}

void MessageBuffer::Writer::removeRange(const char *start, const char *end) {
    std::vector<char> &data = buffer->data;
    char *dataPtr = data.data();
    size_t startIndex = start - dataPtr;
    size_t endIndex = end - dataPtr;
    if (startIndex < buffer->readableSize) {
        buffer->readableSize -=
            std::min(buffer->readableSize, endIndex) - startIndex;
    }
    data.erase(data.begin() + startIndex, data.begin() + endIndex);
}

void MessageBuffer::Writer::end() {
    buffer->isEnd = true;
}

void MessageBuffer::Writer::commit(MessageNotifier &notifier) {
    buffer->readableSize = buffer->data.size() - reservedSize;
    for (int fd : buffer->subscribers) {
        notifier.notifyWrite(fd);
    }
}

} // namespace proxy
