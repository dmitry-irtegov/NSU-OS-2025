#include "jobs.h"
#include "io.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>

#ifndef WTRAPPED
#define WTRAPPED 0 // Linux doesn't have this
#endif

void init_jobs(joblist_t *list) {
    for (int i = 0; i < MAXJOBS; i++) {
        list->jobs[i].state = PROC_EXITED;
        list->jobs[i].prev_index = i - 1;
        list->jobs[i].next_index = i + 1;
    }
    list->jobs[0].prev_index = -1;
    list->jobs[MAXJOBS - 1].next_index = -1;
    list->dead_index = 0;
    list->alive_index = -1;
}

int can_add(joblist_t *list) {
    return list->dead_index != -1;
}

static int jl_add(joblist_t *list, job_t *job) {
    if (list->dead_index == -1) {
        return -1;
    }
    int index = list->dead_index;
    int next_dead_index = list->jobs[index].next_index;
    if (next_dead_index != -1) {
        list->dead_index = next_dead_index;
        list->jobs[next_dead_index].prev_index = -1;
    } else {
        list->dead_index = -1;
    }
    list->jobs[index] = *job;
    list->jobs[index].prev_index = -1;
    list->jobs[index].next_index = list->alive_index;
    if (list->alive_index != -1) {
        list->jobs[list->alive_index].prev_index = index;
    }
    list->alive_index = index;
    return index;
}

static void jl_remove(joblist_t *list, int index) {
    job_t *job = list->jobs + index;
    int prev_index = job->prev_index;
    int next_index = job->next_index;
    if (prev_index != -1) {
        list->jobs[prev_index].next_index = next_index;
    } else {
        list->alive_index = next_index;
    }
    if (next_index != -1) {
        list->jobs[next_index].prev_index = prev_index;
    }
    if (list->dead_index != -1) {
        list->jobs[list->dead_index].prev_index = index;
    }
    job->prev_index = -1;
    job->next_index = list->dead_index;
    list->dead_index = index;
}

static void jl_to_start(joblist_t *list, int index) {
    if (list->alive_index == index) {
        return;
    }
    job_t *job = list->jobs + index;
    int prev_index = job->prev_index;
    int next_index = job->next_index;
    if (prev_index != -1) {
        list->jobs[prev_index].next_index = next_index;
    }
    if (next_index != -1) {
        list->jobs[next_index].prev_index = prev_index;
    }
    if (list->alive_index != -1) {
        list->jobs[list->alive_index].prev_index = index;
    }
    job->prev_index = -1;
    job->next_index = list->alive_index;
    list->alive_index = index;
}

static void print_status(joblist_t *list, int index, FILE *file) {
    job_t *job = list->jobs + index;
    switch (job->state) {
    case PROC_EXITED:
        fprintf(file, "[%d] (%ld): ended\n", index, (long)job->pgid);
        break;
    case PROC_KILLED:
        fprintf(file, "[%d] (%ld): killed\n", index, (long)job->pgid);
        break;
    case PROC_STOPPED:
        fprintf(file, "[%d] (%ld): stopped\n", index, (long)job->pgid);
        break;
    case PROC_RUNNING:
        fprintf(file, "[%d] (%ld): running\n", index, (long)job->pgid);
        break;
    default:
        break;
    }
}

static pstate_t code_to_pstate(int code) {
    switch (code) {
    case CLD_DUMPED:
    case CLD_KILLED:
        return PROC_KILLED;
    case CLD_EXITED:
        return PROC_EXITED;
    case CLD_STOPPED:
    case CLD_TRAPPED:
        return PROC_STOPPED;
    case CLD_CONTINUED:
        return PROC_RUNNING;
    default:
        return PROC_EXITED;
    }
}

static int wait_job(joblist_t *list, int index, char bg) {
    job_t *job = list->jobs + index;
    int count_total = job->count;
    int count_dead = 0;
    int count_stopped = 0;
    int count_killed = 0;

    for (int i = 0; i < count_total; i++) {
        proc_t *proc = job->procs + i;
        switch (proc->state) {
        case PROC_KILLED:
            count_killed++;
        case PROC_EXITED:
            count_dead++;
            continue;
        default:
            break;
        }

        siginfo_t info;
        int wait_options = WEXITED | WSTOPPED | WTRAPPED;
        wait_options |= bg ? WNOHANG | WCONTINUED : 0;
        if (waitid(P_PID, proc->pid, &info, wait_options)) {
            perror("Could not wait for child process");
            return 1;
        }

        if (info.si_pid) {
            proc->state = code_to_pstate(info.si_code);
        }

        switch (proc->state) {
        case PROC_KILLED:
            count_killed++;
        case PROC_EXITED:
            count_dead++;
            break;
        case PROC_STOPPED:
            count_stopped++;
            break;
        default:
            break;
        }
    }

    if (count_dead == count_total) {
        job->state = count_killed ? PROC_KILLED : PROC_EXITED;
        if (bg) {
            print_status(list, index, stderr);
        }
    } else if (count_stopped == count_total - count_dead) {
        if (job->state != PROC_STOPPED) {
            job->state = PROC_STOPPED;
            print_status(list, index, stderr);
        }
    } else {
        job->state = PROC_RUNNING;
    }
    return 0;
}

int wait_background(joblist_t *list) {
    int next_index = 0;
    for (int i = list->alive_index; i != -1; i = next_index) {
        if (wait_job(list, i, 1)) {
            return 1;
        }
        next_index = list->jobs[i].next_index;
        if (list->jobs[i].state & PROC_ANYDEAD) {
            jl_remove(list, i);
        }
    }
    return 0;
}

static int wait_foreground(joblist_t *list, int index) {
    if (wait_job(list, index, 0)) {
        return 1;
    }
    if (set_foreground(0, getpgrp())) {
        return 1;
    }
    pstate_t state = list->jobs[index].state;
    if (state == PROC_STOPPED) {
        if (save_terminal(0, &list->jobs[index].term_attr)) {
            return 1;
        }
    } else if (state == PROC_KILLED) {
        if (restore_terminal(0, &list->jobs[index].term_attr)) {
            return 1;
        }
    }
    if (state & PROC_ANYDEAD) {
        jl_remove(list, index);
    }
    return 0;
}

int add_job(joblist_t *list, job_t *job, char bg) {
    int index = jl_add(list, job);
    if (index == -1) {
        fprintf(stderr, "Could not find a slot for another job.\n");
        return 1;
    }
    if (bg) {
        print_status(list, index, stdout);
        return 0;
    } else {
        return wait_foreground(list, index);
    }
}

int bring_to_foreground(joblist_t *list, int index) {
    if (index == -1) {
        index = list->alive_index;
        if (index == -1) {
            fprintf(stderr, "No background jobs.\n");
            return 0;
        }
    }
    if (index < 0 || index >= MAXJOBS) {
        fprintf(stderr, "Job does not exist.\n");
        return 0;
    }

    job_t *job = list->jobs + index;
    if (job->state & PROC_ANYDEAD) {
        fprintf(stderr, "Job does not exist.\n");
        return 0;
    }

    fprintf(stdout, "Bringing job %d (%ld) to foreground.\n", index,
            (long)job->pgid);
    jl_to_start(list, index);
    job->state = PROC_RUNNING;
    if (restore_terminal(0, &job->term_attr)) {
        return 1;
    }
    if (set_foreground(0, job->pgid)) {
        return 1;
    }
    if (kill(-job->pgid, SIGCONT)) {
        perror("Could not send SIGCONT to job");
        return 1;
    }
    return wait_foreground(list, index);
}

int resume_background(joblist_t *list, int index) {
    if (index == -1) {
        index = list->alive_index;
        if (index == -1) {
            fprintf(stderr, "No background jobs.\n");
            return 0;
        }
    }
    if (index < 0 || index >= MAXJOBS) {
        fprintf(stderr, "Job does not exist.\n");
        return 0;
    }
    job_t *job = list->jobs + index;
    if (job->state & PROC_ANYDEAD) {
        fprintf(stderr, "Job does not exist.\n");
        return 0;
    }
    if (job->state == PROC_RUNNING) {
        fprintf(stderr, "Job is already running.\n");
        return 0;
    }

    fprintf(stdout, "Resuming job %d (%ld).\n", index, (long)job->pgid);
    jl_to_start(list, index);
    job->state = PROC_RUNNING;
    if (kill(-job->pgid, SIGCONT)) {
        perror("Could not send SIGCONT to job");
        return 1;
    }
    return 0;
}

void print_background(joblist_t *list) {
    if (list->alive_index == -1) {
        fprintf(stdout, "No background jobs.\n");
        return;
    }
    fprintf(stdout, "Background jobs:\n");
    for (int i = list->alive_index; i != -1; i = list->jobs[i].next_index) {
        print_status(list, i, stdout);
    }
    return;
}
