#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include "job_control.h"
#include "shell.h"

/* Глобальные переменные для управления заданиями */
static struct job *job_list = NULL;

/* Текущее foreground задание */
static int job_counter = 1;        /* Счетчик для ID заданий */

/* Внешние переменные из shell.c */
extern pid_t shell_pgid;
extern int terminal_fd;
extern pid_t foreground_pgid;

void free_job(struct job *j) {
    if (j->cmdline)
        free(j->cmdline);
    free(j);
}

void put_job_in_foreground(struct job *j, int cont) {
    /* Даем группе процессов управление терминалом */
    tcsetpgrp(terminal_fd, j->pgid);
    foreground_pgid = j->pgid;
    
    /* Если нужно, посылаем SIGCONT для продолжения */
    if (cont) {
        kill(-j->pgid, SIGCONT);
        j->state = 'R';
    }
    
    /* Ждем завершения */
    int status;
    pid_t pid;
    
    while (1) {
        pid = waitpid(-j->pgid, &status, WUNTRACED);
        
        if (pid <= 0) break;
        
        if (WIFSTOPPED(status)) {
            j->state = 'S';
            fprintf(stderr, "\n[%d] Stopped    %s\n", j->jid, j->cmdline);
            break;
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            /* Помечаем процесс как завершенный */
            for (int i = 0; i < j->nprocs; i++) {
                if (j->pids[i] == pid) {
                    j->pids[i] = 0;
                    break;
                }
            }
            
            /* Проверяем, все ли процессы завершены */
            int all_done = 1;
            for (int i = 0; i < j->nprocs; i++) {
                if (j->pids[i] > 0) {
                    if (kill(j->pids[i], 0) == 0 || errno != ESRCH) {
                        all_done = 0;
                        break;
                    }
                }
            }
            
            if (all_done) {
                remove_job(j->pgid);
                break;
            }
        }
    }
    
    /* Возвращаем управление терминалом shell */
    tcsetpgrp(terminal_fd, shell_pgid);
    foreground_pgid = 0;
}

void put_job_in_background(struct job *j, int cont) {
    if (cont) {
        kill(-j->pgid, SIGCONT);
        j->state = 'R';
        fprintf(stderr, "[%d] %d\n", j->jid, j->pgid);
    }
}

struct job *find_job_by_pgid(pid_t pgid) {
    struct job *j;
    for (j = job_list; j; j = j->next) {
        if (j->pgid == pgid)
            return j;
    }
    return NULL;
}

struct job *find_job_by_pid(pid_t pid) {
    struct job *j;
    int i;
    
    for (j = job_list; j; j = j->next) {
        for (i = 0; i < j->nprocs; i++) {
            if (j->pids[i] == pid)
                return j;
        }
    }
    return NULL;
}

struct job *find_job_by_jid(int jid) {
    struct job *j;
    for (j = job_list; j; j = j->next) {
        if (j->jid == jid)
            return j;
    }
    return NULL;
}

struct job *get_job_by_spec(char *spec) {
    if (spec[0] == '%') {
        spec++;
        int jid = atoi(spec);
        return find_job_by_jid(jid);
    } else {
        pid_t pid = atoi(spec);
        struct job *j = find_job_by_pid(pid);
        if (!j) {
            /* Попробуем интерпретировать как PGID */
            j = find_job_by_pgid(pid);
        }
        return j;
    }
}

void do_job_fg(char **args) {
    struct job *j;
    
    if (args[1] == NULL) {
        fprintf(stderr, "fg: usage: fg <jobid>\n");
        return;
    }
    
    j = get_job_by_spec(args[1]);
    if (!j) {
        fprintf(stderr, "fg: job not found: %s\n", args[1]);
        return;
    }
    
    put_job_in_foreground(j, 1);
}

void do_job_bg(char **args) {
    struct job *j;
    
    if (args[1] == NULL) {
        fprintf(stderr, "bg: usage: bg <jobid>\n");
        return;
    }
    
    j = get_job_by_spec(args[1]);
    if (!j) {
        fprintf(stderr, "bg: job not found: %s\n", args[1]);
        return;
    }
    
    put_job_in_background(j, 1);
}

void list_jobs() {
    struct job *j;

    update_job_status();
    
    for (j = job_list; j; j = j->next) {
        fprintf(stderr, "[%d] %c    %s\n", j->jid, j->state, j->cmdline);
    }
}

void add_job(pid_t pgid, int nprocs, pid_t pids[], char *cmdline) {
    struct job *j = malloc(sizeof(struct job));
    j->pgid = pgid;
    j->jid = job_counter++;
    j->state = 'R';
    j->cmdline = cmdline;
    j->nprocs = nprocs;
    
    /* Копируем PID процессов */
    int i;
    for (i = 0; i < nprocs; i++) {
        j->pids[i] = pids[i];
    }
    for (; i < MAXCMDS; i++) {
        j->pids[i] = 0;
    }
    
    j->next = job_list;
    job_list = j;
}

void remove_job(pid_t pgid) {
    struct job *j, *prev = NULL;
    
    for (j = job_list; j; prev = j, j = j->next) {
        if (j->pgid == pgid) {
            if (prev)
                prev->next = j->next;
            else
                job_list = j->next;
            
            free_job(j);
            return;
        }
    }
}

void wait_for_job(struct job *j) {
    int status;
    pid_t pid;
    
    if (!j) return;
    
    /* Ждем завершения всех процессов конвейера */
    while (1) {
        pid = waitpid(-j->pgid, &status, WUNTRACED);
        
        if (pid <= 0) break;
        
        if (WIFSTOPPED(status)) {
            j->state = 'S';
            fprintf(stderr, "\n[%d] Stopped    %s\n", j->jid, j->cmdline);
            break;
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            /* Помечаем процесс как завершенный */
            int i;
            for (i = 0; i < j->nprocs; i++) {
                if (j->pids[i] == pid) {
                    j->pids[i] = 0;
                    break;
                }
            }
            
            /* Проверяем, все ли процессы завершены */
            int all_done = 1;
            for (i = 0; i < j->nprocs; i++) {
                if (j->pids[i] > 0) {
                    if (kill(j->pids[i], 0) == 0 || errno != ESRCH) {
                        all_done = 0;
                        break;
                    }
                }
            }
            
            if (all_done) {
                remove_job(j->pgid);
                break;
            }
        }
    }
}

void update_job_status() {
    int status;
    pid_t pid;
    
    /* Проверяем все завершенные/остановленные процессы */
    while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) {
        struct job *j = find_job_by_pid(pid);
        
        if (!j) continue;
        
        if (WIFSTOPPED(status)) {
            j->state = 'S';
            fprintf(stderr, "\n[%d] Stopped    %s\n", j->jid, j->cmdline);
        } else if (WIFEXITED(status)) {
            fprintf(stderr, "\n[%d] Terminated    %s\n", j->jid, j->cmdline);
            remove_job(j->pgid);
        }
    }
}
