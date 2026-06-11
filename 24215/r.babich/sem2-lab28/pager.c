#include <stdio.h>
#include <unistd.h>
#include "pager.h"

void pager_init(pager_t* pager, buffer_t* buffer, size_t page_size) {
    pager->buffer = buffer;
    pager->offset = 0;
    pager->current_line = 0;
    pager->page_size = page_size;
    pager->paused = false;
}

void pager_display(pager_t *pager) {
    if (pager->paused)
        return;

    buffer_t *buffer = pager->buffer;

    while (pager->offset < buffer->size) {

        char c = buffer->data[pager->offset++];

        write(STDOUT_FILENO, &c, 1);

        if (c == '\n') {
            if (++pager->current_line >= pager->page_size) {
                pager->paused = true;
                pager->current_line = 0;
                write(STDOUT_FILENO, "\nPress space to scroll\n", 23);
                break;
            }
        }
    }
}