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

void sigint_handler(int sig)
{
    printf("\n");
    print_prompt(DEFAULT_SHELL_NAME);
    fflush(stdout);
}

void sigquit_handler(int sig)
{
    printf("\n");
    print_prompt(DEFAULT_SHELL_NAME);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    char line[MAX_LINE];

    signal(SIGINT, sigint_handler);
    signal(SIGQUIT, sigquit_handler);
    signal(SIGTSTP, handle_sigtstp);
    signal(SIGCHLD, handle_sigchld);

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
