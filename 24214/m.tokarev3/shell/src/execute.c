#include "shell.h"

void execute_commands()
{
    if (num_cmds == 1 && is_builtin(cmds[0].cmdargs[0]))
    {
        execute_builtin();
        return;
    }

    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) // Дочерний процесс
    {
        if (bkgrnd)
        {
            signal(SIGINT, SIG_IGN);
            signal(SIGQUIT, SIG_IGN);
        }
        else
        {
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
        }

        setup_redirections();

        if (num_cmds == 1)
        {
            execvp(cmds[0].cmdargs[0], cmds[0].cmdargs);
            perror(cmds[0].cmdargs[0]);
            exit(1);
        }
        else
        {
            execute_pipeline();
        }
    }
    else if (pid > 0) // Родительский процесс
    {
        if (bkgrnd)
        {
            add_job(pid, cmds[0].cmdargs[0]);
            printf("[%d] %d\n", get_job_count(), pid);
        }
        else
        {
            set_foreground_pid(pid);

            signal(SIGINT, SIG_IGN);
            signal(SIGQUIT, SIG_IGN);

            waitpid(pid, &status, WUNTRACED);

            if (WIFSTOPPED(status))
            {
                add_job(pid, cmds[0].cmdargs[0]);
                set_job_status(pid, JOB_STOPPED);
                printf("[%d] Stopped\t\t%s\n", get_job_count(), cmds[0].cmdargs[0]);
            }

            set_foreground_pid(0);

            signal(SIGINT, handle_sigint);
            signal(SIGQUIT, sigquit_handler);
            signal(SIGTSTP, handle_sigtstp);
        }
    }
    else
    {
        perror("fork");
    }
}

void setup_redirections()
{
    if (infile)
    {
        int fd = open(infile, O_RDONLY);
        if (fd < 0)
        {
            perror(infile);
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (outfile)
    {
        int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            perror(outfile);
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    if (appfile)
    {
        int fd = open(appfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd < 0)
        {
            perror(appfile);
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }
}

void execute_pipeline()
{
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];

    // Создаем pipes
    for (int i = 0; i < num_cmds - 1; i++)
    {
        if (pipe(pipes[i]) < 0)
        {
            perror("pipe");
            exit(1);
        }
    }

    for (int i = 0; i < num_cmds; i++)
    {
        pids[i] = fork();
        if (pids[i] == 0)
        {
            if (i > 0)
            {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }
            if (i < num_cmds - 1)
            {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < num_cmds - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            execvp(cmds[i].cmdargs[0], cmds[i].cmdargs);
            perror(cmds[i].cmdargs[0]);
            exit(1);
        }
    }

    for (int i = 0; i < num_cmds - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    for (int i = 0; i < num_cmds; i++)
    {
        int status;
        waitpid(pids[i], &status, 0);
    }
}
