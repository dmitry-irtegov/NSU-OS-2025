#pragma once

#include "job_list.h"
#include "pipeline.h"

typedef void (*builtin_func_t)(command_t *command, job_list_t *list);

typedef struct {
    char *name;         
    builtin_func_t function; 
} builtin_t;

bool try_execute_builtin(pipeline_t *pipeline, job_list_t *list);

