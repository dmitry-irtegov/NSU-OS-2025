#include <unistd.h>

#define MAXARGS 256
#define MAXCMDS 50
#define MAXJOBS 123
#define MAXLINE 1024

typedef enum job_state { RUNNING, STOPPED, DONE } job_state_t;

typedef struct job {
    pid_t pid;
    pid_t pgid;
    int jid;
    job_state_t state;
    char cmdline[MAXLINE];
} job_t;

extern job_t jobs[MAXJOBS];
extern pid_t shell_pgid;
extern int pipes[MAXCMDS][2];

struct command {
    char* cmdargs[MAXARGS];
    char cmdflag;
};

/*  cmdflag's  */
#define OUTPIP 01
#define INPIP 02

extern struct command cmds[];
extern char *infile, *outfile, *appfile;
extern char bkgrnd;

/* parseline.c */
int parseline(char*);

/* promptline.c */
int promptline(char*, char*, int);

/* utils.c */
void handler_child(pid_t pgid, int ncmds, char* cmdline);
void file_operation(char*, int);
void cleanup_zombies();
void set_terminal_foreground(pid_t pgid);

/* signal.c */
void ignore_signals();
void activate_signals();

/* jobs.c */
void init_jobs(void);
int add_job(pid_t pid, pid_t pgid, int state, char* cmdline);
int delete_job(pid_t pid);
struct job* get_job_by_pid(pid_t pid);
struct job* get_job_by_jid(int jid);
void list_jobs(void);

/* pipes.c */
void create_pipes(int pipes[][2], int ncmds);
void setup_pipe_input(int pipes[][2], int cmd_index);
void setup_pipe_output(int pipes[][2], int cmd_index, int ncmds);
void close_all_pipes(int pipes[][2], int ncmds);
void close_unused_pipes(int pipes[][2], int cmd_index, int ncmds);

/* builtin.c */
int execute_builtin(struct command* cmd);
int is_builtin(struct command* cmd);
