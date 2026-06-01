#include "proxy.h"

void buffer_init(Buffer *b) {
    if (b == NULL) {
        return;
    }

    b->data = NULL;
    b->len = 0;
    b->cap = 0;
    b->sent = 0;
}

void buffer_free(Buffer *b) {
    if (b == NULL) {
        return;
    }

    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
    b->sent = 0;
}

void buffer_clear(Buffer *b) {
    if (b == NULL) {
        return;
    }

    b->len = 0;
    b->sent = 0;
}

int buffer_reserve(Buffer *b, size_t need) {
    if (b == NULL) {
        return -1;
    }

    if (need <= b->cap) {
        return 0;
    }

    size_t new_cap = b->cap == 0 ? 4096 : b->cap;

    while (new_cap < need) {
        if (new_cap > ((size_t)-1) / 2) {
            return -1;
        }

        new_cap *= 2;
    }

    char *new_data = realloc(b->data, new_cap);
    if (new_data == NULL) {
        return -1;
    }

    b->data = new_data;
    b->cap = new_cap;

    return 0;
}

int buffer_append(Buffer *b, const void *src, size_t n) {
    if (b == NULL || src == NULL) {
        return -1;
    }

    if (n == 0) {
        return 0;
    }

    if (buffer_reserve(b, b->len + n) < 0) {
        return -1;
    }

    memcpy(b->data + b->len, src, n);
    b->len += n;

    return 0;
}

size_t buffer_unsent_size(const Buffer *b) {
    if (b == NULL || b->sent >= b->len) {
        return 0;
    }

    return b->len - b->sent;
}

char *buffer_unsent_ptr(Buffer *b) {
    if (b == NULL || b->sent >= b->len) {
        return NULL;
    }

    return b->data + b->sent;
}

void buffer_mark_sent(Buffer *b, size_t n) {
    if (b == NULL) {
        return;
    }

    size_t left = buffer_unsent_size(b);

    if (n > left) {
        n = left;
    }

    b->sent += n;
}

void buffer_drop_sent(Buffer *b) {
    if (b == NULL || b->sent == 0) {
        return;
    }

    if (b->sent >= b->len) {
        b->len = 0;
        b->sent = 0;
        return;
    }

    size_t left = b->len - b->sent;
    memmove(b->data, b->data + b->sent, left);

    b->len = left;
    b->sent = 0;
}
