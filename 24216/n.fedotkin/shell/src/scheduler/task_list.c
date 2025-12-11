#include "scheduler/task_list.h"

void init_task_list(task_list_t* list) {
    list->first = NULL;
    list->last = NULL;
    list->count = 0;
    list->next_task_id = 1;
}

void add_task(task_list_t* list, task_t* task) {
    task->task_id = list->next_task_id++;
    task->next = NULL;
    if (list->last) {
        list->last->next = task;
    } else {
        list->first = task;
    }
    list->last = task;
    list->count++;
}

void remove_task(task_list_t* list, task_t* task) {
    task_t *prev = NULL;
    task_t* curr = list->first;
    while (curr) {
        if (curr == task) {
            if (prev) {
                prev->next = curr->next;
            } else {
                list->first = curr->next;
            }
            if (curr == list->last) {
                list->last = prev;
            }
            list->count--;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

task_t* find_task_by_id(task_list_t* list, size_t task_id) {
    task_t* curr = list->first;
    while (curr) {
        if (curr->task_id == task_id) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

void cleanup_completed_tasks(task_list_t* list) {
    task_t *prev = NULL;
    task_t* curr = list->first;
    while (curr) {
        task_t* next = curr->next;
        if (curr->status == TASK_COMPLETED && curr->notify) {
            if (prev) {
                prev->next = curr->next;
            } else {
                list->first = curr->next;
            }
            if (curr == list->last) {
                list->last = prev;
            }
            list->count--;
            destroy_task(curr);
        } else {
            prev = curr;
        }
        curr = next;
    }
    
    if (list->count == 0) {
        list->next_task_id = 1;
    }
}

void print_all_tasks(task_list_t* list) {
    if (list->count == 0) {
        printf("No active jobs\n");
        return;
    }
    
    task_t* curr = list->first;
    while (curr) {
        printf("[%zu]   ", curr->task_id);

        switch (curr->status) {
            case TASK_RUNNING:
                printf("Running                 ");
                break;
            case TASK_STOPPED:
                printf("Stopped                 ");
                break;
            case TASK_COMPLETED:
                printf("Done                    ");
                break;
            default:
                printf("Unknown                 ");
                break;
        }
        
        for (size_t i = 0; i < curr->process_count; i++) {
            if (i > 0) printf(" | ");
            printf("%s", curr->processes[i]->argv[0]);
        }
        
        if (curr->is_background) {
            printf(" &");
        }
        printf("\n");
        
        curr->notify = 1;
        curr = curr->next;
    }
}

void destroy_task_list(task_list_t* list) {
    task_t* curr = list->first;
    while (curr) {
        task_t* next = curr->next;
        destroy_task(curr);
        curr = next;
    }
    
    list->first = NULL;
    list->last = NULL;
    list->count = 0;
}