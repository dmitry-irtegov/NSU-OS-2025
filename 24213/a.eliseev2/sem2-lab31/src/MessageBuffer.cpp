#include "MessageBuffer.h"
#include <cassert>
#include <cstring>

namespace proxy {

MessageBuffer::MessageBuffer() : data(), isEnd(false), readableSize(0) {
}

MessageBuffer::Reader MessageBuffer::read(size_t &position) {
    return Reader(*this, position);
}

MessageBuffer::Writer MessageBuffer::write() {
    return Writer(*this);
}

void MessageBuffer::subscribe(int fd) {
    subscribers.insert(fd);
}

void MessageBuffer::unsubscribe(int fd) {
    subscribers.erase(fd);
}

void MessageBuffer::end(MessageNotifier &notifier) {
    isEnd = true;
    for (int fd : subscribers) {
        notifier.notifyWrite(fd);
    }
}

MessageBuffer::Reader::Reader(MessageBuffer &buffer, size_t &position)
    : buffer(&buffer), position(&position) {
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
}

std::vector<char> &MessageBuffer::Writer::data() {
    return buffer->data;
}

char *MessageBuffer::Writer::reserve(size_t size) {
    size_t oldSize = buffer->data.size() - reservedSize;
    buffer->data.resize(oldSize + size);
    reservedSize = size;
    return buffer->data.data() + oldSize;
}

void MessageBuffer::Writer::written(size_t size) {
    buffer->data.resize(buffer->data.size() - reservedSize + size);
    reservedSize = 0;
}

void MessageBuffer::Writer::appendRange(const char *data, size_t size) {
    assert(reservedSize == 0 && "Write already in progress");
    std::vector<char> &bufData = buffer->data;
    bufData.reserve(bufData.size() + size);
    bufData.insert(bufData.end(), data, data + size);
}

void MessageBuffer::Writer::insertRange(const char *data, size_t size,
                                        size_t at) {
    assert(reservedSize == 0 && "Write already in progress");
    std::vector<char> &bufData = buffer->data;
    bufData.reserve(bufData.size() + size);
    bufData.insert(bufData.begin() + at, data, data + size);
}

void MessageBuffer::Writer::removeRange(size_t at, size_t size) {
    assert(reservedSize == 0 && "Write already in progress");
    std::vector<char> &data = buffer->data;
    data.erase(data.begin() + at, data.begin() + at + size);
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
