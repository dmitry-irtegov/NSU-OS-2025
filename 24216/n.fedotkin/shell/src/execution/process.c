#include "execution/process.h"

process_t* create_process(command_t* cmd) {
    process_t* process = malloc(sizeof(process_t));
    if (!process) {
        return NULL;
    }
    process->pid = -1;

    process->argv = malloc((cmd->argc + 1) * sizeof(char*));
    if (!process->argv) { 
        free(process);
        return NULL;
    }

    for (size_t i = 0; i < cmd->argc; i++) {
        process->argv[i] = strdup(cmd->argv[i]);
    }
    process->argv[cmd->argc] = NULL;

    process->argc = cmd->argc + 1;

    process->status = PROCESS_RUNNING;

    return process;
}

void destroy_process(process_t* process) {
    if (process) {
        for (size_t i = 0; i < process->argc - 1; i++) {
            free(process->argv[i]);
        }
        free(process->argv);
        free(process);
    }
}