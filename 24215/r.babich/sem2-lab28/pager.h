#pragma once

#include <stdbool.h>
#include "buffer.h"

typedef struct {
    buffer_t* buffer;
    size_t offset;
    size_t current_line;
    size_t page_size;
    bool paused;
} pager_t;

void pager_init(pager_t* pager, buffer_t* buffer, size_t page_size);
void pager_display(pager_t* pager);