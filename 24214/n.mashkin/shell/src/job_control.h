#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <unistd.h>
#include "shell.h"

/* Структура для хранения информации о заданиях */
typedef struct job {
    pid_t pgid;          /* PGID группы процессов конвейера */
    int jid;            /* ID задания (1, 2, 3, ...) */
    char state;         /* 'R' - running, 'S' - stopped */
    char *cmdline;      /* Командная строка */
    int nprocs;         /* Количество процессов в конвейере */
    pid_t pids[MAXCMDS]; /* PID процессов в конвейере */
    struct job *next;
} job_t;

void put_job_in_foreground(job_t *);
void put_job_in_background(job_t *);
job_t *get_job_by_spec(char *);

job_t *find_job_by_pid(pid_t);
job_t *find_job_by_pgid(pid_t);
void add_job(pid_t, int, pid_t[], char *);
void remove_job(pid_t);
void wait_for_job(job_t *);
void update_job_status();

extern job_t *job_list;

#endif
