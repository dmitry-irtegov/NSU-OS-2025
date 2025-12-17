#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include "job_control.h"
#include "shell.h"

job_t *job_list = NULL;
int job_counter = 1;

void free_job(job_t *j) {
    if (j->cmdline) {
        free(j->cmdline);
    }
    free(j);
}

void put_job_in_foreground(job_t *j) {
    tcsetpgrp(terminal_fd, foreground_pgid = j->pgid);
    
    kill(-j->pgid, SIGCONT);
    j->state = 'R';
    
    wait_for_job(j);
    
    tcsetpgrp(terminal_fd, shell_pgid);
    foreground_pgid = 0;
}

void put_job_in_background(job_t *j) {
    kill(-j->pgid, SIGCONT);
    j->state = 'R';
    fprintf(stderr, "[%d] %d\n", j->jid, j->pgid);
}

job_t *find_job_by_pgid(pid_t pgid) {
    for (job_t *j = job_list; j; j = j->next) {
        if (j->pgid == pgid)
            return j;
    }
    return NULL;
}

job_t *find_job_by_pid(pid_t pid) {
    for (job_t *j = job_list; j; j = j->next) {
        for (int i = 0; i < j->nprocs; i++) {
            if (j->pids[i] == pid)
                return j;
        }
    }
    return NULL;
}

job_t *find_job_by_jid(int jid) {
    for (job_t *j = job_list; j; j = j->next) {
        if (j->jid == jid)
            return j;
    }
    return NULL;
}

job_t *get_job_by_spec(char *spec) {
    if (!spec) {
        return job_list;
    }

    if (spec[0] == '%') {
        spec++;
        int jid = atoi(spec);
        return find_job_by_jid(jid);
    } else {
        pid_t pid = atoi(spec);
        job_t *j = find_job_by_pid(pid);
        if (!j) {
            j = find_job_by_pgid(pid);
        }
        return j;
    }
}

void add_job(pid_t pgid, int bkgrnd, int nprocs, pid_t pids[], char *cmdline) {
    job_t *j = malloc(sizeof(job_t));
    j->pgid = pgid;
    j->bkgrnd = bkgrnd;
    j->jid = job_counter++;
    j->state = 'R';
    j->cmdline = cmdline;
    j->nprocs = nprocs;
    
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
    if (!job_list) {
        return;
    }
    if (job_list->pgid == pgid) {
        job_t *tmp = job_list;
        job_list = job_list->next;
        free_job(tmp);
        return;
    }

    for (job_t *j = job_list; j->next; j = j->next) {
        if (j->next->pgid == pgid) {
            job_t *tmp = j->next;
            j->next = j->next->next;
            free_job(tmp);
            return;
        }
    }
}

void wait_for_job(job_t *j) {
    if (!j) return;
    
    while (1) {
        int status;
        pid_t pid = waitpid(-j->pgid, &status, WUNTRACED);
        
        if (pid <= 0) break;
        
        if (WIFSTOPPED(status)) {
            j->state = 'S';
            fprintf(stderr, "\n[%d] Stopped\n", j->jid);
            break;
        } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
            for (int i = 0; i < j->nprocs; i++) {
                if (j->pids[i] == pid) {
                    j->pids[i] = 0;
                    break;
                }
            }
            
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
}

void update_job_status() {
    int status;
    pid_t pid;
    
    while ((pid = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0) {
        job_t *j = find_job_by_pid(pid);
        
        if (!j) continue;
        
        if (WIFSTOPPED(status)) {
            j->state = 'S';
            fprintf(stderr, "\n[%d] Stopped\n", j->jid);
        } else if (WIFEXITED(status)) {
            int jid = j->jid;
            remove_job(j->pgid);
            if (!j->bkgrnd) {
                continue;
            }
            fprintf(stderr, "\n[%d] Terminated\n", jid);
        }
    }
}
