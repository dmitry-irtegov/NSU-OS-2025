#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define DEFAULT_SHELL_NAME "mega_shell_3000"

#define MAX_LINE 1024
#define MAX_ARGS 100
#define MAX_CMDS 100
#define MAX_JOBS 100

typedef enum
{
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} job_status_t;

typedef struct
{
    pid_t pid;
    int job_id;
    char command[MAX_LINE];
    job_status_t status;
} job_t;

typedef struct command
{
    char *cmdargs[MAX_ARGS];
} command_t;

extern command_t cmds[MAX_CMDS];

extern char *infile;
extern char *outfile;
extern char *appfile;

extern int bkgrnd;

extern int num_cmds;

// parseline.c
int parseline(char *line);

// execute.c
void execute_commands();

// execute.c
void setup_redirections();
void execute_pipeline();

// jobs.c
void initialize_jobs();

void handle_sigtstp(int sig);
void handle_sigchld(int sig);

void add_job(pid_t pid, char *command);
int get_job_count();

void check_jobs();
void print_jobs();
void cleanup_jobs();
void set_job_status(pid_t pid, job_status_t status);
void set_foreground_pid(pid_t pid);

int find_job_by_id(int job_id);
int find_latest_stopped_job();

void fg_handler();
void bg_handler();

// builtin.c
int is_builtin(char *arg);
void execute_builtin();

// signal handlers
void sigint_handler(int sig);
void sigquit_handler(int sig);

// shell.c
void print_prompt(char *name);
