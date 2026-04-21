#pragma once

#include <set>
#include <vector>

namespace proxy {

class MessageNotifier {
  public:
    virtual void notifyWrite(int fd) = 0;
};

class MessageBuffer {
  public:
    class Reader {
      public:
        Reader(MessageBuffer &buffer, size_t &position);

        Reader(const Reader &) = delete;
        Reader &operator=(const Reader &) = delete;

        const char *data();
        size_t length();
        bool isEnd();
        void advance(size_t size);

      private:
        MessageBuffer *buffer;
        size_t *position;
    };

    class Writer {
      public:
        Writer(MessageBuffer &buffer);

        Writer(const Writer &) = delete;
        Writer &operator=(const Writer &) = delete;

        std::vector<char> &data();

        char *reserve(size_t size);
        void written(size_t size);

        void appendRange(const char *data, size_t size);
        void insertRange(const char *data, size_t size, size_t at);
        void removeRange(size_t at, size_t size);
        void end();

        void commit(MessageNotifier &notifier);

      private:
        MessageBuffer *buffer;
        size_t reservedSize;
    };

    MessageBuffer();

    Reader read(size_t &position);
    Writer write();
    void end(MessageNotifier &notifier);

    void subscribe(int fd);
    void unsubscribe(int fd);

  private:
    std::vector<char> data;
    std::set<int> subscribers;
    bool isEnd;
    size_t readableSize;
};

} // namespace proxy
