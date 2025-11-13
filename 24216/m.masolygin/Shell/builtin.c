#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"

int builtin_exit(struct command* cmd) {
    exit(0);
    return 0;
}

int builtin_jobs(struct command* cmd) {
    list_jobs();
    return 0;
}

int builtin_fg(struct command* cmd) {
    int jid;
    struct job* jb;

    if (cmd->cmdargs[1] != NULL) {
        if (cmd->cmdargs[1][0] == '%') {
            jid = atoi(&cmd->cmdargs[1][1]);
        } else {
            jid = atoi(cmd->cmdargs[1]);
        }
        jb = get_job_by_jid(jid);
    } else {
        jb = NULL;
        for (int i = MAXJOBS - 1; i >= 0; i--) {
            if (jobs[i].pid != 0) {
                jb = &jobs[i];
                break;
            }
        }
    }

    if (jb == NULL || jb->pid == 0) {
        fprintf(stderr, "fg: no such job\n");
        return -1;
    }

    printf("%s\n", jb->cmdline);

    if (jb->state == STOPPED) {
        kill(-jb->pgid, SIGCONT);
    }
    jb->state = RUNNING;

    set_terminal_foreground(jb->pgid);

    handler_child(jb->pgid, 1, jb->cmdline);

    set_terminal_foreground(shell_pgid);

    return 0;
}

int builtin_bg(struct command* cmd) {
    int jid;
    struct job* jb;

    if (cmd->cmdargs[1] != NULL) {
        if (cmd->cmdargs[1][0] == '%') {
            jid = atoi(&cmd->cmdargs[1][1]);
        } else {
            jid = atoi(cmd->cmdargs[1]);
        }
        jb = get_job_by_jid(jid);
    } else {
        jb = NULL;
        for (int i = MAXJOBS - 1; i >= 0; i--) {
            if (jobs[i].pid != 0 && jobs[i].state == STOPPED) {
                jb = &jobs[i];
                break;
            }
        }
    }

    if (jb == NULL || jb->pid == 0) {
        fprintf(stderr, "bg: no such job\n");
        return -1;
    }

    if (jb->state != STOPPED) {
        fprintf(stderr, "bg: job already running\n");
        return -1;
    }

    printf("[%d]   %s &\n", jb->jid, jb->cmdline);

    kill(-jb->pgid, SIGCONT);
    jb->state = RUNNING;

    return 0;
}

typedef struct builtin_cmd {
    char* name;
    int (*func)(struct command*);
} builtin_cmd_t;

static builtin_cmd_t builtin_cmds[] = {{"exit", builtin_exit},
                                       {"jobs", builtin_jobs},
                                       {"fg", builtin_fg},
                                       {"bg", builtin_bg},
                                       {NULL, NULL}};

int execute_builtin(struct command* cmd) {
    if (cmd == NULL || cmd->cmdargs[0] == NULL) {
        return -1;
    }

    for (int i = 0; builtin_cmds[i].name != NULL; i++) {
        if (strcmp(cmd->cmdargs[0], builtin_cmds[i].name) == 0) {
            return builtin_cmds[i].func(cmd);
        }
    }

    return -1;
}

int is_builtin(struct command* cmd) {
    if (cmd == NULL || cmd->cmdargs[0] == NULL) {
        return 0;
    }

    for (int i = 0; builtin_cmds[i].name != NULL; i++) {
        if (strcmp(cmd->cmdargs[0], builtin_cmds[i].name) == 0) {
            return 1;
        }
    }

    return 0;
}
