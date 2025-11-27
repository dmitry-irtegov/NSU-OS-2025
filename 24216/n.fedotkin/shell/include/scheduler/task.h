#pragma once

#include <sys/types.h>
#include <stddef.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "execution/process.h"
#include "execution/pipeline.h"
#include "core/error_handler.h"

typedef enum {
    TASK_NOT_LAUNCHED,
    TASK_RUNNING,
    TASK_STOPPED,
    TASK_COMPLETED
} task_status_t;

typedef struct task_t{
    size_t task_id;
    pid_t pgid;

    process_t** processes;
    size_t process_count;

    char* input_file;
    char* output_file;
    char* append_file;

    int is_background;

    task_status_t status;

    struct termios tmodes;
    int notify;

    struct task_t* next;
} task_t;

task_t* create_task(pipeline_t pipeline);
void destroy_task(task_t* task);



