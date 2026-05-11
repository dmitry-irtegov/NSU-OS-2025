#pragma once

#include <pthread.h>
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
        ~Reader();

        Reader(const Reader &) = delete;
        Reader &operator=(const Reader &) = delete;
        Reader(Reader &&) = delete;
        Reader &operator=(Reader &&) = delete;

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
        ~Writer();

        Writer(const Writer &) = delete;
        Writer &operator=(const Writer &) = delete;
        Writer(Writer &&) = delete;
        Writer &operator=(Writer &&) = delete;

        std::vector<char> &data();

        char *allocate(size_t size);
        void written(size_t size);
        
        void reserve(size_t size);

        void appendRange(const char *data, size_t size);
        void insertRange(const char *data, size_t size, size_t at);
        void removeRange(size_t at, size_t size);
        void end();

        void commit(MessageNotifier &notifier);

      private:
        MessageBuffer *buffer;
        size_t allocSize;
    };

    MessageBuffer();
    ~MessageBuffer();
    
    MessageBuffer(const MessageBuffer &) = delete;
    MessageBuffer &operator=(const MessageBuffer &) = delete;
    MessageBuffer(MessageBuffer &&) = delete;
    MessageBuffer &operator=(MessageBuffer &&) = delete;

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

    pthread_mutex_t lock;
};

} // namespace proxy
