#pragma once

#include <stdlib.h>

typedef struct {
    unsigned char* data;
    size_t size;
    size_t capacity;
} buffer_t;

void buffer_init(buffer_t* buffer);
void buffer_free(buffer_t* buffer);
void buffer_append(buffer_t* buffer, const unsigned char* data, size_t size);