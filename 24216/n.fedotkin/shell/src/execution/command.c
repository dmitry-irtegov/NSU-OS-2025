#include "execution/command.h"

command_t* create_command() {
    command_t* cmd = malloc(sizeof(command_t));
    if (!cmd) {
        return NULL;
    }

    cmd->argv = NULL;
    cmd->argc = 0;
    cmd->cap = 0;

    return cmd;
}

void destroy_command(command_t* cmd) {
    if (cmd) {
        for (size_t i = 0; i < cmd->argc; i++) {
            free(cmd->argv[i]);
        }
        free(cmd->argv);
        free(cmd);
    }
}

void push_command_arg(command_t* cmd, char* arg) {
    if (cmd->cap == 0) {
        cmd->argv = malloc(sizeof(char*));
        cmd->cap = 1;
    } else if (cmd->argc >= cmd->cap) {
        cmd->cap *= 2;
        cmd->argv = realloc(cmd->argv, cmd->cap * sizeof(char*));
    }
    cmd->argv[cmd->argc++] = strdup(arg);
}