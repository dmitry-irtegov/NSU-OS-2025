#include "shell.h"

static job_t jobs[MAX_JOBS];
static int job_count = 0;
static pid_t foreground_pgid = 0;

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

        // Получаем управление терминалом
        tcsetpgrp(shell_terminal, shell_pgid);

        // Сохраняем атрибуты терминала по умолчанию
        tcgetattr(shell_terminal, &shell_tmodes);
    }
}

void initialize_jobs()
{
    job_count = 0;
    for (int i = 0; i < MAX_JOBS; i++)
    {
        jobs[i].pid = 0;
        jobs[i].pgid = 0;
        jobs[i].job_id = 0;
        jobs[i].command[0] = '\0';
        jobs[i].status = JOB_RUNNING;
        jobs[i].tmodes_set = 0;
    }
}

void add_job(pid_t pid, pid_t pgid, char *command)
{
    if (pid <= 0 || command == NULL)
    {
        return;
    }

    if (job_count < MAX_JOBS)
    {
        jobs[job_count].pid = pid;
        jobs[job_count].pgid = pgid;
        jobs[job_count].job_id = job_count + 1;
        strncpy(jobs[job_count].command, command, sizeof(jobs[job_count].command) - 1);
        jobs[job_count].command[sizeof(jobs[job_count].command) - 1] = '\0';
        jobs[job_count].status = JOB_RUNNING;
        jobs[job_count].tmodes_set = 0;
        job_count++;
    }
    else
    {
        fprintf(stderr, "Warning: job table full, cannot add job %d\n", pid);
    }
}

int get_job_count()
{
    return job_count;
}

void check_jobs()
{
    int status;
    pid_t pid;

    for (int i = 0; i < job_count; i++)
    {
        pid = waitpid(-jobs[i].pgid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (pid > 0)
        {
            if (WIFEXITED(status) || WIFSIGNALED(status))
            {
                printf("[%d] Done\t\t%s\n", jobs[i].job_id, jobs[i].command);

                for (int j = i; j < job_count - 1; j++)
                {
                    jobs[j] = jobs[j + 1];
                }
                job_count--;
                i--;
            }
            else if (WIFSTOPPED(status))
            {
                if (jobs[i].status != JOB_STOPPED)
                {
                    jobs[i].status = JOB_STOPPED;

                    if (jobs[i].pgid == foreground_pgid && shell_is_interactive)
                    {
                        tcgetattr(shell_terminal, &jobs[i].tmodes);
                        jobs[i].tmodes_set = 1;
                    }
                }
            }
            else if (WIFCONTINUED(status))
            {
                if (jobs[i].status == JOB_STOPPED)
                {
                    jobs[i].status = JOB_RUNNING;
                    printf("[%d] Continued\t\t%s\n", jobs[i].job_id, jobs[i].command);
                }
            }
        }
    }
}

void set_foreground_job(pid_t pgid)
{
    foreground_pgid = pgid;
}

void put_job_in_foreground(job_t *j, int cont)
{
    if (!shell_is_interactive)
        return;

    tcsetpgrp(shell_terminal, j->pgid);

    if (j->tmodes_set)
    {
        tcsetattr(shell_terminal, TCSADRAIN, &j->tmodes);
    }

    if (cont)
    {
        if (kill(-j->pgid, SIGCONT) < 0)
        {
            perror("kill (SIGCONT)");
        }
    }

    set_foreground_job(j->pgid);
    int status;
    waitpid(-j->pgid, &status, WUNTRACED);

    tcsetpgrp(shell_terminal, shell_pgid);

    if (WIFSTOPPED(status))
    {
        tcgetattr(shell_terminal, &j->tmodes);
        j->tmodes_set = 1;
        j->status = JOB_STOPPED;
    }

    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);

    set_foreground_job(0);

    if (WIFEXITED(status) || WIFSIGNALED(status))
    {
        for (int i = 0; i < job_count; i++)
        {
            if (&jobs[i] == j)
            {
                for (int k = i; k < job_count - 1; k++)
                {
                    jobs[k] = jobs[k + 1];
                }
                job_count--;
                break;
            }
        }
    }
}

void put_job_in_background(job_t *j, int cont)
{
    if (cont)
    {
        if (kill(-j->pgid, SIGCONT) < 0)
        {
            perror("kill (SIGCONT)");
        }
    }
}

void set_job_status(pid_t pid, job_status_t status)
{
    if (pid <= 0)
    {
        return;
    }

    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].pid == pid)
        {
            jobs[i].status = status;
            return;
        }
    }
}

int find_job_by_id(int job_id)
{
    if (job_id <= 0)
    {
        return -1;
    }

    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].job_id == job_id)
        {
            return i;
        }
    }
    return -1;
}

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
    // Простой обработчик - основная работа делается в check_jobs()
    // Это более переносимое решение
}

void cleanup_jobs()
{
    printf("Terminating background jobs...\n");

    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].pgid > 0)
        {
            kill(-jobs[i].pgid, SIGTERM);
        }
    }

    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].pgid > 0)
        {
            int status;
            pid_t result = waitpid(-jobs[i].pgid, &status, WNOHANG);
            if (result == 0)
            {
                kill(-jobs[i].pgid, SIGKILL);
                waitpid(-jobs[i].pgid, &status, 0);
            }
        }
    }

    job_count = 0;
}

void print_jobs()
{
    if (job_count == 0)
    {
        printf("No active jobs.\n");
        return;
    }

    for (int i = 0; i < job_count; i++)
    {
        const char *status_str;
        switch (jobs[i].status)
        {
        case JOB_RUNNING:
            status_str = "Running";
            break;
        case JOB_STOPPED:
            status_str = "Stopped";
            break;
        case JOB_DONE:
            status_str = "Done";
            break;
        default:
            status_str = "Unknown";
            break;
        }

        printf("[%d] %s\t\t%s (pgid %d)\n",
               jobs[i].job_id,
               status_str,
               jobs[i].command,
               jobs[i].pgid);
    }
}

void fg_handler()
{
    int job_index = -1;

    if (cmds[0].cmdargs[1] != NULL)
    {
        int job_id = atoi(cmds[0].cmdargs[1]);
        if (job_id <= 0)
        {
            printf("fg: invalid job number: %s\n", cmds[0].cmdargs[1]);
            return;
        }
        job_index = find_job_by_id(job_id);
        if (job_index == -1)
        {
            printf("fg: job %d not found\n", job_id);
            return;
        }
    }
    else
    {
        printf("fg: no job's index\n");
        return;
    }

    if (job_index < 0 || job_index >= job_count)
    {
        printf("fg: internal error - invalid job index\n");
        return;
    }

    printf("%s\n", jobs[job_index].command);
    put_job_in_foreground(&jobs[job_index], 1);
}

void bg_handler()
{
    int job_index = -1;

    if (cmds[0].cmdargs[1] != NULL)
    {
        int job_id = atoi(cmds[0].cmdargs[1]);
        if (job_id <= 0)
        {
            printf("bg: invalid job number: %s\n", cmds[0].cmdargs[1]);
            return;
        }
        job_index = find_job_by_id(job_id);
        if (job_index == -1)
        {
            printf("bg: job %d not found\n", job_id);
            return;
        }
    }
    else
    {
        printf("bg: no job's index\n");
        return;
    }

    if (job_index < 0 || job_index >= job_count)
    {
        printf("bg: internal error - invalid job index\n");
        return;
    }

    if (jobs[job_index].status != JOB_STOPPED)
    {
        printf("bg: job %d is already running\n", jobs[job_index].job_id);
        return;
    }

    jobs[job_index].status = JOB_RUNNING;
    put_job_in_background(&jobs[job_index], 1);
    printf("[%d] %s &\n", jobs[job_index].job_id, jobs[job_index].command);
}
