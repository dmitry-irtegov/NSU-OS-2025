#pragma once
#include <stddef.h>
#include <stdlib.h>
#include "execution/command.h"

typedef struct {
    command_t** cmds;
    size_t count;
    size_t cap;
    char* input_file;
    char* output_file;
    char* append_file;
    int is_background;
} pipeline_t;

pipeline_t* create_pipeline();
void destroy_pipeline(pipeline_t* pipeline);
void push_pipeline_command(pipeline_t* pipeline, command_t* cmd);
