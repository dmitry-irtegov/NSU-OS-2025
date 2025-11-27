#include "parser/task_manager.h"

task_manager_t* create_task_manager() {
    task_manager_t* manager = malloc(sizeof(task_manager_t));
    if (!manager) {
        return NULL;
    }
    manager->tasks = NULL;
    manager->count = 0;
    manager->cap = 0;
    return manager;
}

void destroy_task_manager(task_manager_t* manager) {
    if (manager) {
        for (size_t i = 0; i < manager->count; i++) {
            destroy_pipeline(manager->tasks[i]);
        }
        free(manager->tasks);
        free(manager);
    }
}

void add_pipeline_task_manager(task_manager_t* manager, pipeline_t* task) {
    if (manager->cap == 0) {
        manager->cap = 1;
        manager->tasks = malloc(manager->cap * sizeof(pipeline_t*));
    } else if (manager->count == manager->cap) {
        size_t new_cap = manager->cap * 2;
        manager->tasks = realloc(manager->tasks, new_cap * sizeof(pipeline_t*));
        manager->cap = new_cap;
    }
    manager->tasks[manager->count++] = task;
}