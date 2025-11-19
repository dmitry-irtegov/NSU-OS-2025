#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "jobs.h"
#include <errno.h>

static Job* job1 = NULL;

const char* state_str(State state) {
    switch (state) {
        case RUNNING:
            return "Running";
        case STOPPED:
            return "Stopped";
        case SIGNALED:
            return "Signaled";
        case DONE:
            return "Done";
        default:
            return "Udefined";
    }
}

Proc* create_proc(int pid, char* prompt) {
    Proc* proc = (Proc*)calloc(1, sizeof(Proc));
    if (!proc) {
        perror("calloc failed");
        exit(EXIT_FAILURE);
    }
    proc->pid = pid;
    proc->state = RUNNING;
    strcpy(proc->prompt, prompt);
    return proc;
}

Job* create_job(int pgid, State state, char* prompt, Proc* procs) {
    int job_num = 1;
    Job* current = job1;
    Job* last_job = NULL;
    Job* new_job = (Job*)calloc(1, sizeof(Job));

    while(current) {
        if (job_num == current->number) {
            job_num++;
        } else {
            break;
        }
        last_job = current;
        current = current->next;
    }

    if(last_job == NULL) {
        job1 = new_job;
    } else {
        last_job->next = new_job;
        new_job->prev = last_job;
    }

    if (current != NULL) {
        new_job->next = current;
        current->prev = new_job;
    }

    strcpy(new_job->prompt, prompt);

    new_job->procs = procs;
    new_job->pgid = pgid;
    new_job->state = state;
    new_job->number = job_num;
    new_job->fg = 0;
    new_job->ready = 0;
    
    return new_job;
}

void print_job(Job* job) {
    if (job->fg) {
        return;
    }

    const char* suffix  = (job->state == RUNNING || job->state == STOPPED) ? "&" : "";

    fprintf(stderr, 
            "[%d] %c %-10s %s%s\n", 
            job->number, 
            job->ready ? '*' : ' ', 
            state_str(job->state),
            job->prompt, 
            suffix);
            
    job->ready = 0; 
}

void print_all_jobs() {
    Job* current = job1;
    if (current == NULL) {
        fprintf(stderr, "No jobs.\n");
        return;
    }
    while(current) {
        print_job(current);
        current = current->next;
    }
}

Job* parsing_job(char* line) {
    if (line == NULL) {
        fprintf(stderr, "jobs: no job specified\n");
        return NULL;
    }
    else if (line[0] == '%') {
        if (line[1] == '\0') {
            fprintf(stderr, "jobs: invalid job spec\n");
            return NULL;
        }

        int job_number = atoi(line + 1);
        if (job_number < 1) {
            fprintf(stderr, "jobs: job not found: %s\n", line);
            return NULL;
        }
        Job* job = get_job_by_jid(job_number);
        if (job == NULL) {
            fprintf(stderr, "jobs: job not found: %s\n", line);
        }
        return job;
    }
    fprintf(stderr, "jobs: invalid job spec: %s\n", line);
    return NULL;
}

Job* get_first_job() {
    return job1;
}

Job* get_job_by_jid(int jid) {
    Job* job = job1;
    while (job) {
        if (job->number == jid) {
            return job;
        }
        job = job->next;
    }
    return NULL;
}

Job* get_job_by_pgid(int pgid) {
    Job* job = job1;
    while (job) {
        if (job->pgid == pgid) {
            return job;
        }
        job = job->next;
    }
    return NULL;
}

void state_update(Job* job) {
    if (job->fg || dead_job(job->state)) {
        return;
    }

    State last_state = job->state;

    int all_done_or_signaled = 1;
    int any_stopped = 0;
    int any_running = 0;

    for (Proc* current = job->procs; current; current = current->next) {
        if (dead_job(current->state)) {
            continue;
        }
        
        int status = 0;
        switch (waitpid(current->pid, &status, WNOHANG | WUNTRACED | WCONTINUED)) {
            case 0:
                break;
            case -1:
                if (errno == ECHILD) {
                    current->state = DONE;
                } else { 
                    perror("waitpid() failed");
                    current->state = DONE;
                }
                break;
            default:
                if (WIFEXITED(status)) {
                    current->state = DONE;
                } else if (WIFSIGNALED(status)) {
                    current->state = SIGNALED;
                } else if (WIFSTOPPED(status)) {
                    current->state = STOPPED;
                } else if (WIFCONTINUED(status)) {
                    current->state = RUNNING;
                }
        }

        if (current->state == RUNNING) {
            any_running = 1;
        }
        if (current->state == STOPPED) {
            any_stopped = 1;
        }
        if (current->state != DONE && current->state != SIGNALED) {
            all_done_or_signaled = 0;
        }
    }
    
    if (any_running) {
        job->state = RUNNING;
    } else if (any_stopped) {
        job->state = STOPPED;
    } else if (all_done_or_signaled) {
        job->state = DONE;
    }

    if (last_state != job->state) {
        job->ready = 1;
    }
}

void check_update_states() {
    Job* current = job1;
    while (current != NULL) {
        state_update(current); 
        
        if (current->ready) {
            print_job(current);
        }

        if (dead_job(current->state)) { 
            if (current->ready) {
                print_job(current);
            }
            current = deleting_job(current); 
        } else {
            current = current->next;
        }
    }
}

void switch_to_bg(Job* job) {
    if (job == NULL) {
        return;
    }

    if (kill(-job->pgid, SIGCONT) == -1) {
        perror("kill(SIGCONT) failed");
        return;
    }
    job->state = RUNNING;
    job->fg = 0;
    print_job(job);
}

void switch_to_fg(Job* job) {
    if (job == NULL) {
        return;
    }
    job->state = RUNNING;
    job->fg = 1;
}

void waiting_fg(Job* job) {
    if (job == NULL) return;
  
    job->fg = 1;
    Proc* current = job->procs;
    while (current) {
        if (dead_job(current->state)) {
            current = current->next;
            continue;
        }

        int status = 0;
        pid_t pid;
        do {
            pid = waitpid(current->pid, &status, WUNTRACED);
        } while (pid == -1 && errno == EINTR);

        if (pid < 0) {
            if (errno != ECHILD) {
                 perror("waitpid() failed");
            }
            current->state = DONE;
            current = current->next;
            continue;
        }
        
        if (pid == current->pid) {

             if (WIFEXITED(status)) {
                current->state = DONE; 
            } else if (WIFSIGNALED(status)) {
                fprintf(stderr, "\nJob [%d] signaled (%d)\n", job->number, WTERMSIG(status));
                current->state = SIGNALED;
            } else if (WIFSTOPPED(status)) {
                fprintf(stderr, "\nJob [%d] stopped (%d)\n", job->number, WSTOPSIG(status));
                job->state = current->state = STOPPED;
                job->ready = 1;
                job->fg = 0;
                return;
            }
        }
        
        current = current->next;
    }   
    deleting_job(job);
}

int dead_job(State state) {
    if (state == DONE || state == SIGNALED) {
        return 1;
    }
    return 0;
}

Job* deleting_job(Job* job) {
    if (job == NULL) return NULL;

    Job* temp_next = job->next;
    Proc* temp_curr = job->procs;
    
    if (job == job1) {
        job1 = job->next;
    }
    if (job->prev) {
        job->prev->next = job->next;
    }
    if(job->next) {
        job->next->prev = job->prev;
    }

    while(temp_curr) {
        Proc* temp_prev_proc = temp_curr;
        temp_curr = temp_curr->next;
        free(temp_prev_proc);
    }
    
    free(job);
    
    return temp_next;
}

void kill_job() {
    Job* current = job1;
    while(current) {
        state_update(current);
        if (dead_job(current->state) == 0) {
            kill(-current->pgid, SIGKILL);
        }
        current = deleting_job(current);
    }
    job1 = NULL;
}