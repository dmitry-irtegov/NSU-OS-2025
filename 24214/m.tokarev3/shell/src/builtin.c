#include "shell.h"

int is_builtin(char *arg)
{
    if (!arg)
        return 0;

    if (strcmp(arg, "exit") == 0 ||
        strcmp(arg, "fg") == 0 ||
        strcmp(arg, "bg") == 0 ||
        strcmp(arg, "jobs") == 0 ||
        strcmp(arg, "cd") == 0)
    {
        return 1;
    }
    return 0;
}

void execute_builtin()
{
    char *cmd = cmds[0].cmdargs[0];

    if (strcmp(cmd, "exit") == 0)
    {
        cleanup_jobs();
        printf("Goodbye!\n");
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(cmd, "fg") == 0)
    {
        fg_handler();
    }
    else if (strcmp(cmd, "bg") == 0)
    {
        bg_handler();
    }
    else if (strcmp(cmd, "jobs") == 0)
    {
        print_jobs();
    }
    else if (strcmp(cmd, "cd") == 0)
    {
        cd_handler();
    }
}

void cd_handler()
{
    char *dir = cmds[0].cmdargs[1];

    if (dir == NULL)
    {
        dir = getenv("HOME");
        if (dir == NULL)
        {
            fprintf(stderr, "cd: HOME environment variable not set\n");
            return;
        }
    }

    if (chdir(dir) != 0)
    {
        perror("cd");
    }
}
