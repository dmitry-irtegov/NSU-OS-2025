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

char *get_prompt()
{
    static char prompt[MAX_LINE];
    char *pwd = getenv("PWD");
    char *user = getenv("USER");
    if (user && pwd)
    {
        snprintf(prompt, MAX_LINE, "%s:%s ", user, pwd);
    }
    else
    {
        snprintf(prompt, MAX_LINE, "[%s] ", DEFAULT_SHELL_NAME);
    }
    return prompt;
}

void clear_globals()
{
    if (infile)
    {
        free(infile);
        infile = NULL;
    }
    if (outfile)
    {
        free(outfile);
        outfile = NULL;
    }
    if (appfile)
    {
        free(appfile);
        appfile = NULL;
    }
    bkgrnd = 0;
    num_cmds = 0;
    for (int i = 0; i < MAX_CMDS; i++)
    {
        for (int j = 0; j < MAX_ARGS; j++)
        {
            if (cmds[i].cmdargs[j])
            {
                free(cmds[i].cmdargs[j]);
                cmds[i].cmdargs[j] = NULL;
            }
        }
    }
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
    char *line;
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
        print_done_jobs();

        line = readline(get_prompt());

        if (!line)
        {
            printf("\n");
            cleanup_jobs();
            printf("Goodbye!\n");
            break;
        }

        if (*line)
            add_history(line);

        char *next_cmd = line;
        while (next_cmd && *next_cmd)
        {
            num_cmds = parseline(&next_cmd);

            if (num_cmds == -1)
            {
                fprintf(stderr, "Parsing error\n");
                break;
            }

            if (num_cmds == 0)
            {
                break;
            }

            if (is_builtin(cmds[0].cmdargs[0]))
            {
                if (execute_builtin())
                {
                    free(line);
                }
            }
            else
            {
                execute_commands();
            }
        }
        free(line);
    }

    return 0;
}
