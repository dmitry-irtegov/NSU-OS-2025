#include "shell.h"

struct command cmds[MAX_CMDS];
char *infile = NULL;
char *outfile = NULL;
char *appfile = NULL;
int bkgrnd = 0;
int num_cmds = 0;

void print_prompt(char *name)
{
    printf("[%s] ", name);
    fflush(stdout);
}

void sigquit_handler(int sig)
{
}

int main(int argc, char **argv)
{
    char line[MAX_LINE];
    struct sigaction sa;

    // Настройка обработчика SIGINT (Ctrl+C)
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    // Настройка обработчика SIGQUIT (Ctrl+\)
    sa.sa_handler = sigquit_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGQUIT, &sa, NULL);

    // Настройка обработчика SIGTSTP (Ctrl+Z)
    sa.sa_handler = handle_sigtstp;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL);

    // Настройка обработчика SIGCHLD
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
            printf("\n");
            cleanup_jobs();
            printf("Goodbye!\n");
            break;
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
