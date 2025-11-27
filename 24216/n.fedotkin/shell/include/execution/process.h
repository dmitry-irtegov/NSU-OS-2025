#pragma once
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "execution/command.h"

typedef enum {
    PROCESS_RUNNING,
    PROCESS_COMPLETED,
    PROCESS_STOPPED,
} process_status_t;

typedef struct process {
    pid_t pid;
    char** argv;
    size_t argc;
    process_status_t status;
} process_t;

process_t* create_process(command_t* cmd);
void destroy_process(process_t* process);
