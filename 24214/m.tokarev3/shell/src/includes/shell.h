#pragma once

#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>

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
    pid_t pgid;
    int job_id;
    char command[MAX_LINE];
    job_status_t status;
    struct termios tmodes;
    int tmodes_set;
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

// Terminal management
extern struct termios shell_tmodes;
extern int shell_terminal;
extern int shell_is_interactive;
extern pid_t shell_pgid;
extern pid_t foreground_pgid;

// parseline.c
int parseline(char **line);

// execute.c
void execute_commands();
void execute_pipeline();

// shell.c
void init_shell();
void clear_globals();

// signals.c
void handle_sigtstp(int sig);
void handle_sigint(int sig);
void handle_sigchld(int sig);

// jobs.c
void initialize_jobs();
void add_job(pid_t pid, pid_t pgid, char *command);
int get_job_count();
void check_jobs();
void print_done_jobs();
void print_jobs();
void cleanup_jobs();
void set_job_status(pid_t pid, job_status_t status);
void set_foreground_job(pid_t pgid);
void put_job_in_foreground(job_t *j, int cont);
void put_job_in_background(job_t *j, int cont);
void fg_handler();
void bg_handler();

// builtin.c
int is_builtin(char *arg);
int execute_builtin();
