#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "shell.h"

job_t jobs[MAXJOBS];
pid_t shell_pgid;

void init_jobs(void) {
    for (int i = 0; i < MAXJOBS; i++) {
        jobs[i].pid = 0;
        jobs[i].pgid = 0;
        jobs[i].jid = 0;
        jobs[i].state = DONE;
        jobs[i].cmdline[0] = '\0';
    }
}

int add_job(pid_t pid, pid_t pgid, int state, char* cmdline) {
    if (pid <= 0 || cmdline == NULL) {
        return -1;
    }

    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].pgid = pgid;
            jobs[i].jid = i + 1;
            jobs[i].state = state;
            strncpy(jobs[i].cmdline, cmdline, MAXLINE - 1);
            jobs[i].cmdline[MAXLINE - 1] = '\0';
            return jobs[i].jid;
        }
    }

    fprintf(stderr, "Error: job list is full\n");
    return -1;
}
int delete_job(pid_t pid) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == pid) {
            jobs[i].pid = 0;
            jobs[i].pgid = 0;
            jobs[i].jid = 0;
            jobs[i].state = DONE;
            jobs[i].cmdline[0] = '\0';
            return 0;
        }
    }
    return -1;
}

struct job* get_job_by_pid(pid_t pid) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid == pid) {
            return &jobs[i];
        }
    }
    return NULL;
}

struct job* get_job_by_jid(int jid) {
    if (jid < 1 || jid > MAXJOBS) return NULL;

    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].jid == jid) {
            return &jobs[i];
        }
    }
    return NULL;
}

void list_jobs(void) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].pid != 0) {
            char* state_str;
            switch (jobs[i].state) {
                case RUNNING:
                    state_str = "Running";
                    break;
                case STOPPED:
                    state_str = "Stopped";
                    break;
                case DONE:
                    state_str = "Done";
                    break;
                default:
                    state_str = "Unknown";
            }
            printf("[%d]   %-10s %s\n", jobs[i].jid, state_str,
                   jobs[i].cmdline);
        }
    }
}