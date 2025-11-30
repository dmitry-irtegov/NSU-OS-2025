#include "builtins.h"
#include "jobs.h"
#include "pipeline.h"
#include <errno.h>
#include <limits.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char *name;
    int (*run)(const command_t *, joblist_t *);
} builtin_t;

static int builtin_fg(const command_t *, joblist_t *);
static int builtin_bg(const command_t *, joblist_t *);
static int builtin_cd(const command_t *, joblist_t *);
static int builtin_jobs(const command_t *, joblist_t *);

static const builtin_t builtins[] = {
    {"cd", builtin_cd},
    {"fg", builtin_fg},
    {"bg", builtin_bg},
    {"jobs", builtin_jobs},
    {NULL},
};

int try_builtin(pipeline_t *pipeline, joblist_t *jobs, char *is_error) {
    if (pipeline->cmd_count != 1) {
        return 0;
    }
    if (pipeline->flags || pipeline->in_file || pipeline->out_file) {
        return 0;
    }
    for (const builtin_t *builtin = builtins; builtin->name; builtin++) {
        if (strcmp(builtin->name, pipeline->commands[0].args[0]) == 0) {
            if (builtin->run(pipeline->commands, jobs)) {
                *is_error = 1;
            }
            return 1;
        }
    }
    return 0;
}

static int parse_int(char *str, int *result) {
    char *end_ptr;
    errno = 0;
    long strtol_res = strtol(str, &end_ptr, 10);
    if (str == end_ptr || errno) {
        return 1;
    }
    if (strtol_res > INT_MAX) {
        return 1;
    }
    if (strtol_res < INT_MIN) {
        return 1;
    }
    *result = strtol_res;
    return 0;
}

static int builtin_fg(const command_t *command, joblist_t *jobs) {
    int index = -1;
    if (command->args[1]) {
        if (parse_int(command->args[1], &index) || index < 0) {
            fprintf(stderr, "fg: Job number must be a nonnegative integer.\n");
            return 0;
        }
    }
    return bring_to_foreground(jobs, index);
}

static int builtin_bg(const command_t *command, joblist_t *jobs) {
    int index = -1;
    if (command->args[1]) {
        if (parse_int(command->args[1], &index) || index < 0) {
            fprintf(stderr, "bg: Job number must be a nonnegative integer.\n");
            return 0;
        }
    }
    return resume_background(jobs, index);
}

static int builtin_cd(const command_t *command, joblist_t *jobs) {
    char *dir;
    if (command->args[1]) {
        dir = command->args[1];
    } else {
        struct passwd *passwd = getpwuid(getuid());
        if (!passwd) {
            perror("cd: getpwuid failed");
            return 0;
        }
        dir = passwd->pw_dir;
    }
    if (chdir(dir)) {
        perror("cd: Could not change directory");
    }
    return 0;
}

static int builtin_jobs(const command_t *command, joblist_t *jobs) {
    print_background(jobs);
    return 0;
}
