#include "shell.h"

struct command cmds[MAX_CMDS];
char *infile = NULL;
char *outfile = NULL;
char *appfile = NULL;
int bkgrnd = 0;
int num_cmds = 0;

struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;
pid_t shell_pgid;

void print_prompt(char *name)
{
    printf("[%s] ", name);
    fflush(stdout);
}

void init_shell()
{
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if (shell_is_interactive)
    {
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(-shell_pgid, SIGTTIN);

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, SIG_IGN);

        shell_pgid = getpid();
        if (setpgid(shell_pgid, shell_pgid) < 0)
        {
            perror("Couldn't put the shell in its own process group");
            exit(1);
        }

        tcsetpgrp(shell_terminal, shell_pgid);

        tcgetattr(shell_terminal, &shell_tmodes);
    }
}

int main(int argc, char **argv)
{
    char line[MAX_LINE];
    struct sigaction sa;

    init_shell();

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGQUIT, &sa, NULL);

    sa.sa_handler = handle_sigtstp;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL);

    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    initialize_jobs();

    while (1)
    {
        check_jobs();

        print_prompt(DEFAULT_SHELL_NAME);

        if (fgets(line, MAX_LINE, stdin) == NULL)
        {
            if (feof(stdin))
            {
                printf("\n");
                cleanup_jobs();
                printf("Goodbye!\n");
                break;
            }
            else if (ferror(stdin))
            {
                clearerr(stdin);
                continue;
            }
        }

        num_cmds = parseline(line);

        if (num_cmds == -1)
        {
            fprintf(stderr, "Parsing error\n");
            continue;
        }

        if (num_cmds == 0)
        {
            continue;
        }

        if (is_builtin(cmds[0].cmdargs[0]))
        {
            execute_builtin();
            continue;
        }

        execute_commands();
    }

    return 0;
}
