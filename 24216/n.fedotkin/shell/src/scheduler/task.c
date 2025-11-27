#include "scheduler/task.h"

task_t* create_task(pipeline_t pipeline) {
    task_t* task = malloc(sizeof(task_t));
    if (!task) {
        set_error(ERROR_MEMORY_ALLOCATION, "Failed to create task");
        return NULL;
    }
    task->task_id = 0;
    task->pgid = 0;
    task->process_count = pipeline.count;
    task->is_background = pipeline.is_background;

    task->status = TASK_NOT_LAUNCHED;
    task->notify = 0;
    task->next = NULL;

    task->processes = malloc(sizeof(process_t*) * pipeline.count);
    if (!task->processes) {
        free(task);
        set_error(ERROR_MEMORY_ALLOCATION, "Failed to create task processes");
        return NULL;
    }
    for (size_t i = 0; i < pipeline.count; i++) {
        task->processes[i] = create_process(pipeline.cmds[i]);
    }

    tcgetattr(STDIN_FILENO, &task->tmodes);

    task->input_file = pipeline.input_file ? strdup(pipeline.input_file) : NULL;
    task->output_file = pipeline.output_file ? strdup(pipeline.output_file) : NULL;
    task->append_file = pipeline.append_file ? strdup(pipeline.append_file) : NULL;

    return task;
}

void destroy_task(task_t* task) {
    if (task) {
        for (size_t i = 0; i < task->process_count; i++) {
            destroy_process(task->processes[i]);
        }
        free(task->processes);
        free(task->input_file);
        free(task->output_file);
        free(task->append_file);
        free(task);
    }
}
