#include "shell.h"

void handle_sigtstp(int sig)
{
    if (foreground_pgid > 0 && foreground_pgid != shell_pgid)
    {
        if (kill(-foreground_pgid, SIGTSTP) == 0)
        {
            printf("\n");
        }
        else
        {
            perror("kill");
            printf("\n");
            print_prompt(DEFAULT_SHELL_NAME);
            fflush(stdout);
        }
    }
    else
    {
        printf("\n");
        print_prompt(DEFAULT_SHELL_NAME);
        fflush(stdout);
    }
}

void handle_sigint(int sig)
{
    if (foreground_pgid > 0 && foreground_pgid != shell_pgid)
    {
        if (kill(-foreground_pgid, SIGINT) == 0)
        {
            printf("\n");
        }
        else
        {
            perror("kill");
            printf("\n");
            print_prompt(DEFAULT_SHELL_NAME);
            fflush(stdout);
        }
    }
    else
    {
        printf("\n");
        print_prompt(DEFAULT_SHELL_NAME);
        fflush(stdout);
    }
}

void handle_sigchld(int sig)
{
    // Пустой обработчик для прерывания блокирующих вызовов
    // Основная обработка в check_jobs()
}
