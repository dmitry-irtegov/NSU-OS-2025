#pragma once
#include <string.h>

#define MAX_PROMPT_LEN (1024)

typedef enum State_t {
    RUNNING, 
    STOPPED,
    SIGNALED,  
    DONE
} State;

typedef struct Proc_t Proc;
typedef struct Job_t Job;

struct Job_t {
    int number;
    int pgid;
    unsigned char fg;
    struct Job_t* prev, *next;
    State state;
    unsigned char ready;
    char prompt[MAX_PROMPT_LEN];
    Proc* procs;
};

struct Proc_t {
    int pid;
    struct Proc_t *next;
    State state;
    char prompt[MAX_PROMPT_LEN];
};

void sigchld_handler(int sig);

Job* create_job(int pgid, State state, char* prompt, Proc* procs);
void print_job(Job* job);
void print_all_jobs();
Job* parsing_job(char* line);
Job* get_first_job();
Job* get_job_by_jid(int jid);
Job* get_job_by_pgid(int pgid);
void state_update(Job* job);
void check_update_states();
void switch_to_bg(Job* job);
void switch_to_fg(Job* job);
void waiting_fg(Job* job);
int dead_job(State state);
Job* deleting_job(Job* job);
void kill_job();

Proc* create_proc(int pid, char* prompt);