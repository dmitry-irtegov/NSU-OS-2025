#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    ERROR_NONE,
    ERROR_UNMATCHED_QUOTES,
    ERROR_INVALID_REDIRECTION,
    ERROR_EMPTY_COMMAND,
    ERROR_EMPTY_PIPELINE,
    ERROR_MEMORY_ALLOCATION,
    ERROR_SYNTAX,
    ERROR_UNKNOWN,
} error_t;

typedef struct {
    error_t type;
    char* message;
} error_info_t;

extern error_info_t error_info;

void set_error(error_t type, const char* message);
void clear_error();
int has_error();
void print_error();