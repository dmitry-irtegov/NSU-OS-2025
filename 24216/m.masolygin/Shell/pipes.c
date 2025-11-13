#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "shell.h"

void create_pipes(int pipes[][2], int ncmds) {
    for (int i = 0; i < ncmds - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }
}

void setup_pipe_input(int pipes[][2], int cmd_index) {
    if (cmd_index > 0) {
        if (dup2(pipes[cmd_index - 1][0], STDIN_FILENO) == -1) {
            perror("dup2 - pipe input");
            exit(1);
        }
    }
}

void setup_pipe_output(int pipes[][2], int cmd_index, int ncmds) {
    if (cmd_index < ncmds - 1) {
        if (dup2(pipes[cmd_index][1], STDOUT_FILENO) == -1) {
            perror("dup2 - pipe output");
            exit(1);
        }
    }
}

void close_all_pipes(int pipes[][2], int ncmds) {
    for (int i = 0; i < ncmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}

void close_unused_pipes(int pipes[][2], int cmd_index, int ncmds) {
    for (int i = 0; i < ncmds - 1; i++) {
        if (i != cmd_index - 1) {
            close(pipes[i][0]);
        }
        if (i != cmd_index) {
            close(pipes[i][1]);
        }
    }
}