#pragma once
#include <stddef.h>
#include <stdlib.h>
#include "execution/pipeline.h"

typedef struct {
    pipeline_t** tasks;
    size_t count;
    size_t cap;
} task_manager_t;

task_manager_t* create_task_manager();
void destroy_task_manager(task_manager_t* manager);
void add_pipeline_task_manager(task_manager_t* manager, pipeline_t* task);