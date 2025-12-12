#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <unistd.h>
#include "shell.h"

/* Структура для хранения информации о заданиях */
struct job {
    pid_t pgid;          /* PGID группы процессов конвейера */
    int jid;            /* ID задания (1, 2, 3, ...) */
    char state;         /* 'R' - running, 'S' - stopped */
    char *cmdline;      /* Командная строка */
    int nprocs;         /* Количество процессов в конвейере */
    pid_t pids[MAXCMDS]; /* PID процессов в конвейере */
    struct job *next;
};

void do_job_fg(char **args);
void do_job_bg(char **args);
void list_jobs(); 
struct job *find_job_by_pid(pid_t pid);
struct job *find_job_by_pgid(pid_t pgid);
void add_job(pid_t pgid, int nprocs, pid_t pids[], char *cmdline);
void remove_job(pid_t pgid);
void wait_for_job(struct job *j);
void update_job_status();

#endif
