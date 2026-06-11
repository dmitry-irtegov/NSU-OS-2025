#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buffer.h"

void buffer_init(buffer_t* buffer) {
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}

void buffer_append(buffer_t* buffer, const unsigned char* data, size_t size) {
    if (buffer->size + size > buffer->capacity) {
        size_t new_capacity = (buffer->capacity == 0) ? 1024 : buffer->capacity * 2;
        while (new_capacity < buffer->size + size) {
            new_capacity *= 2;
        }
        unsigned char* new_data = realloc(buffer->data, new_capacity);
        if (!new_data) {
            fprintf(stderr, "Failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }
        buffer->data = new_data;
        buffer->capacity = new_capacity;
    }
    memcpy(buffer->data + buffer->size, data, size);
    buffer->size += size;
}


void buffer_free(buffer_t* buffer) {
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    buffer->capacity = 0;
}