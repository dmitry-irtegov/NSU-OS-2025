#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

typedef struct Reader {
    char *buffer;
    size_t buffer_size;
    size_t chars_read;
    int in_quotes;
    int is_multiline;
} reader_t;

char *read_line();