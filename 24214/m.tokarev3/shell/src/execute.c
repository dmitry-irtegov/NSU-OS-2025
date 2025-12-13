#include "shell.h"

static void setup_redirections()
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

void execute_commands()
{
    if (num_cmds == 1 && is_builtin(cmds[0].cmdargs[0]))
    {
        execute_builtin();
        return;
    }

    if (num_cmds > 1)
    {
        execute_pipeline();
        return;
    }

    pid_t pid, pgid = 0;
    int status;
    char command_line[MAX_LINE];

    strcpy(command_line, cmds[0].cmdargs[0]);
    if (bkgrnd)
    {
        strcat(command_line, " &");
    }

    pid = fork();
    if (pid == 0) // Дочерний процесс
    {
        pid = getpid();
        if (pgid == 0)
            pgid = pid;
        setpgid(pid, pgid);

        if (!bkgrnd && shell_is_interactive)
        {
            tcsetpgrp(shell_terminal, pgid);
        }

        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);

        setup_redirections();

        execvp(cmds[0].cmdargs[0], cmds[0].cmdargs);
        perror(cmds[0].cmdargs[0]);
        exit(1);
    }
    else if (pid > 0) // Родительский процесс
    {
        if (pgid == 0)
            pgid = pid;
        setpgid(pid, pgid);

        if (bkgrnd)
        {
            add_job(pid, pgid, command_line);
            printf("[%d] %d\n", get_job_count(), pid);
        }
        else
        {
            if (shell_is_interactive)
            {
                tcsetpgrp(shell_terminal, pgid);
            }

            set_foreground_job(pgid);

            waitpid(pid, &status, WUNTRACED);

            if (shell_is_interactive)
            {
                tcsetpgrp(shell_terminal, shell_pgid);
            }

            if (WIFSTOPPED(status))
            {
                add_job(pid, pgid, command_line);
                set_job_status(pid, JOB_STOPPED);
                printf("[%d] Stopped\t\t%s\n", get_job_count(), command_line);
            }

            set_foreground_job(0);
        }
    }
    else
    {
        perror("fork");
    }
}

void execute_pipeline()
{
    int pipes[MAX_CMDS - 1][2];
    pid_t pids[MAX_CMDS];
    pid_t pgid = 0;

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
            pid_t pid = getpid();

            if (pgid == 0)
                pgid = pid;
            setpgid(pid, pgid);

            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGTTIN, SIG_DFL);
            signal(SIGTTOU, SIG_DFL);

            if (i == 0 && infile)
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
            else if (i > 0)
            {
                dup2(pipes[i - 1][0], STDIN_FILENO);
            }

            if (i == num_cmds - 1)
            {
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
                else if (appfile)
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
            else if (i < num_cmds - 1)
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
        else if (pids[i] > 0)
        {
            if (pgid == 0)
                pgid = pids[i];
            setpgid(pids[i], pgid);
        }
        else
        {
            perror("fork");
            exit(1);
        }
    }

    for (int i = 0; i < num_cmds - 1; i++)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Передаем управление терминалом группе процессов pipeline
    if (!bkgrnd && shell_is_interactive)
    {
        tcsetpgrp(shell_terminal, pgid);
    }

    char command_line[MAX_LINE];
    strcpy(command_line, cmds[0].cmdargs[0]);
    if (bkgrnd)
    {
        strcat(command_line, " &");
    }

    if (bkgrnd)
    {
        add_job(pgid, pgid, command_line);
        printf("[%d] %d\n", get_job_count(), pgid);
    }
    else
    {
        set_foreground_job(pgid);
    }

    for (int i = 0; i < num_cmds; i++)
    {
        int status;
        waitpid(pids[i], &status, WUNTRACED);

        if (WIFSTOPPED(status))
        {
            if (!bkgrnd)
            {
                add_job(pgid, pgid, command_line);
                set_job_status(pgid, JOB_STOPPED);
                printf("[%d] Stopped\t\t%s\n", get_job_count(), command_line);
            }
        }
    }

    // Возвращаем управление терминалом обратно shell
    if (!bkgrnd && shell_is_interactive)
    {
        tcsetpgrp(shell_terminal, shell_pgid);
        set_foreground_job(0);
    }
}
