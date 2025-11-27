#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "scheduler/task.h"


typedef struct {
    task_t* first;
    task_t* last;
    size_t count;
    size_t next_task_id;
} task_list_t;

void init_task_list(task_list_t* list);
void add_task(task_list_t* list, task_t* task);
void remove_task(task_list_t* list, task_t* task);
task_t* find_task_by_id(task_list_t* list, size_t task_id);
void cleanup_completed_tasks(task_list_t* list);
void print_all_tasks(task_list_t* list);
void destroy_task_list(task_list_t* list);

