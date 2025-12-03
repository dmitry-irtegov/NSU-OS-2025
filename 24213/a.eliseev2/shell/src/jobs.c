#include "jobs.h"
#include "io.h"

#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>

#ifndef WTRAPPED
#define WTRAPPED 0 // Linux doesn't have this
#endif

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

static void jl_move_start(joblist_t *list, int index) {
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

static void update_proc_state(job_t *job, siginfo_t *siginfo) {
    if (siginfo->si_pid == 0) {
        return;
    }

    pstate_t state;
    switch (siginfo->si_code) {
    case CLD_DUMPED:
    case CLD_KILLED:
        state = PROC_KILLED;
        break;
    case CLD_EXITED:
        state = PROC_EXITED;
        break;
    case CLD_STOPPED:
    case CLD_TRAPPED:
        state = PROC_STOPPED;
        break;
    case CLD_CONTINUED:
        state = PROC_RUNNING;
        break;
    default:
        state = PROC_EXITED;
        break;
    }

    for (int i = 0; i < job->proc_count; i++) {
        proc_t *proc = job->procs + i;
        if (proc->pid != siginfo->si_pid) {
            continue;
        }
        proc->state = state;
        break;
    }
}

static void update_job_state(job_t *job) {
    job->state = PROC_EXITED;
    for (int i = 0; i < job->proc_count; i++) {
        proc_t *proc = job->procs + i;

        switch (proc->state) {
        case PROC_STOPPED:
            job->state = PROC_STOPPED;
            continue;
        case PROC_KILLED:
            if (job->state == PROC_EXITED) {
                job->state = PROC_KILLED;
            }
            continue;
        case PROC_EXITED:
            continue;
        default:
            job->state = PROC_RUNNING;
            break;
        }
        break;
    }
}

static int wait_job(joblist_t *list, int index, char bg) {
    job_t *job = list->jobs + index;

    int wait_options = WEXITED | WSTOPPED | WTRAPPED;
    wait_options |= bg ? WNOHANG | WCONTINUED : 0;

    pstate_t old_state = job->state;

    siginfo_t siginfo;
    do {
        if (waitid(P_PGID, job->pgid, &siginfo, wait_options)) {
            perror("Could not wait for child process");
            return 1;
        }
        update_proc_state(job, &siginfo);
        update_job_state(job);
    } while (job->state == PROC_RUNNING && siginfo.si_pid);

    if (job->state == PROC_STOPPED && old_state != PROC_STOPPED) {
        print_status(list, index, stderr);
    }
    if (job->state & PROC_ANYDEAD && bg) {
        print_status(list, index, stderr);
    }

    if (job->state & PROC_ANYDEAD) {
        jl_remove(list, index);
    }
    return 0;
}

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

int add_job(joblist_t *list, job_t *job, char bg) {
    int index = jl_add(list, job);
    if (index == -1) {
        fprintf(stderr, "Could not find a slot for another job.\n");
        return 1;
    }

    if (bg) {
        print_status(list, index, stdout);
        return 0;
    }
    if (wait_job(list, index, 0)) {
        return 1;
    }
    if (set_foreground(getpgid(getpid()))) {
        return 1;
    }
    // Save terminal attributes if the job exited successfully,
    // revert if the job got killed or stopped
    if (list->jobs[index].state == PROC_EXITED){
        save_terminal();
    } else {
        restore_terminal();
    }
    return 0;
}

int wait_background(joblist_t *list) {
    int next_index = 0;
    for (int i = list->alive_index; i != -1; i = next_index) {
        next_index = list->jobs[i].next_index;
        if (wait_job(list, i, 1)) {
            return 1;
        }
    }
    return 0;
}

int bring_to_foreground(joblist_t *list, int index) {
    if (index == -1) {
        index = list->alive_index;
        if (index == -1) {
            fprintf(stdout, "There are no jobs.\n");
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

    fprintf(stdout, "[%d] (%ld): sent to foreground\n", index,
            (long)job->pgid);
    jl_move_start(list, index);
    job->state = PROC_RUNNING;

    save_terminal();
    if (set_foreground(job->pgid)) {
        return 1;
    }

    kill(-job->pgid, SIGCONT);
    if (wait_job(list, index, 0)) {
        return 1;
    }

    if (set_foreground(getpgid(getpid()))) {
        return 1;
    }
    restore_terminal();
    return 0;
}

int resume_background(joblist_t *list, int index) {
    if (index == -1) {
        index = list->alive_index;
        if (index == -1) {
            fprintf(stdout, "There are no jobs.\n");
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

    jl_move_start(list, index);
    job->state = PROC_RUNNING;
    print_status(list, index, stdout);
    if (kill(-job->pgid, SIGCONT)) {
        perror("Could not send SIGCONT to job");
        return 1;
    }
    return 0;
}

void print_background(joblist_t *list) {
    if (list->alive_index == -1) {
        fprintf(stdout, "There are no jobs.\n");
        return;
    }
    for (int i = list->alive_index; i != -1; i = list->jobs[i].next_index) {
        print_status(list, i, stdout);
    }
    return;
}
