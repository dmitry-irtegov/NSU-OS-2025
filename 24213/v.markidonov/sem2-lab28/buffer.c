#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

void extend(buffer *b, int size) {
    if (b->capacity - b->offset - b->size >= size) {
        return;
    }

    int extend_size = b->capacity < size ? size*2 : b->capacity*2;
    b->buf = (char*)realloc(b->buf, extend_size);
    if (!b->buf) {
        perror("realloc");
        exit(1);
    }
    b->capacity = extend_size;
}

void buffer_init(buffer *b, int capacity) {
    b->buf = (char*)malloc(capacity);
    if (!b->buf) {
        perror("malloc");
        exit(1);
    }
    b->capacity = capacity;
    b->size = 0;
    b->offset = 0;
}

int buffer_recv(int sock_fd, buffer *b, int size) {
    extend(b, size);
    int n = recv(sock_fd, b->buf + b->offset + b->size, size, 0);
    if (n < 0) {
        perror("recv");
        exit(1);
    }
    b->size += n;
    return n;
}

int buffer_write(int fd, buffer *b, int size) {
    int n = write(fd, b->buf + b->offset, size);
    if (n < 0) {
        perror("write");
        exit(1);
    }
    b->offset += n;
    b->size -= n;

    if (b->size == 0) {
        b->offset = 0;
    }

    return n;
}

char buffer_getchar(buffer *b) {
    char c = b->buf[b->offset++];
    b->size--;
    return c;
}
