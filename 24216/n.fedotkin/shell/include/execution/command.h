#pragma once
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
    char** argv;
    size_t argc;
    size_t cap;
} command_t;

command_t* create_command();
void destroy_command(command_t* cmd);
void push_command_arg(command_t* cmd, char* arg);
