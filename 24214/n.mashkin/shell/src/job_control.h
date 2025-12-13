#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <unistd.h>
#include "shell.h"

typedef struct job {
    pid_t pgid;
    int jid;
    int bkgrnd;
    char state;
    char *cmdline;
    int nprocs;
    pid_t pids[MAXCMDS];
    struct job *next;
} job_t;

void put_job_in_foreground(job_t *);
void put_job_in_background(job_t *);
job_t *get_job_by_spec(char *);

job_t *find_job_by_pid(pid_t);
job_t *find_job_by_pgid(pid_t);
void add_job(pid_t, int, int, pid_t[], char *);
void remove_job(pid_t);
void wait_for_job(job_t *);
void update_job_status();

extern job_t *job_list;

#endif
