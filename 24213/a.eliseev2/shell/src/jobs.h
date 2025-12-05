#ifndef __JOBS_H
#define __JOBS_H

#include "parse.h"
#include "shell_limits.h"

#include <sys/types.h>
#include <termios.h>

typedef enum {
    PROC_RUNNING = 0,
    PROC_STOPPED = 1,
    PROC_EXITED = 2,
    PROC_KILLED = 4,

    PROC_ANYDEAD = PROC_EXITED | PROC_KILLED,
} pstate_t;

typedef struct {
    pid_t pid;
    pstate_t state;
} proc_t;

typedef struct {
    proc_t procs[MAXCMD];
    int proc_count;
    pid_t pgid;
    pstate_t state;
    int prev_index;
    int next_index;
} job_t;

typedef struct {
    job_t jobs[MAXJOBS];
    int dead_index;
    int alive_index;
} joblist_t;

int launch_job(pipeline_t *pipeline, job_t *job);

void init_jobs(joblist_t *list);
int can_add(joblist_t *list);
int add_job(joblist_t *list, job_t *job, char bg);
int wait_background(joblist_t *list);
int bring_to_foreground(joblist_t *list, int index);
int resume_background(joblist_t *list, int index);
void print_background(joblist_t *list);

#endif // __JOBS_H
