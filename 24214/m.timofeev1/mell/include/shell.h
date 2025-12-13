#include <sys/types.h>

#define MAXARGS 256
#define MAXCMDS 50
#define MAXJOBS 100

struct command
{
	char *cmdargs[MAXARGS];
	char cmdflag;
};

#define OUTPIP 01
#define INPIP 02

#define JOB_RUNNING 1
#define JOB_STOPPED 2
#define JOB_DONE 3

struct job
{
	pid_t pgid;
	int state;
	char *cmdline;
};

extern struct command cmds[];
extern char *infile, *outfile, *appfile;
extern char bkgrnd;

int parseline(char *);
int promptline(char *, char *, int);

int add_job(pid_t pgid, int state, char *cmdline);
int find_job(pid_t pgid);
void cleanup_jobs();
void sigchld_handler(int sig);
void update_job_state(int job_idx, int state);
char *get_job_cmdline(int job_idx);
int has_running_jobs();
void wait_for_jobs();

void sigint_handler(int sig);
void init_signals(void);
void setup_child_signals(void);

int builtin_fg(void);
int builtin_bg(void);
int builtin_cd(char **args);
int builtin_jobs(void);
int builtin_exit(void);
int execute_builtin(char *cmd, char **args);
int is_builtin(char *cmd);

int execute_pipeline(int ncmds, char *cmdline);

void setup_io_redirection(int cmd_idx, int ncmds, int pipes[][2]);
void close_all_pipes(int npipes, int pipes[][2]);
void execute_command(int cmd_idx);