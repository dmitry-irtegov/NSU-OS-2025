#include "shell.h"

static job_t jobs[MAX_JOBS];
static int job_count = 0;
static pid_t foreground_pid = 0;

void initialize_jobs()
{
    job_count = 0;
    for (int i = 0; i < MAX_JOBS; i++)
    {
        jobs[i].pid = 0;
        jobs[i].job_id = 0;
        jobs[i].command[0] = '\0';
        jobs[i].status = JOB_RUNNING;
    }
}

void add_job(pid_t pid, char *command)
{
    if (pid <= 0 || command == NULL)
    {
        return;
    }

    if (job_count < MAX_JOBS)
    {
        jobs[job_count].pid = pid;
        jobs[job_count].job_id = job_count + 1;
        strncpy(jobs[job_count].command, command, sizeof(jobs[job_count].command) - 1);
        jobs[job_count].command[sizeof(jobs[job_count].command) - 1] = '\0';
        jobs[job_count].status = JOB_RUNNING;
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
        pid = waitpid(jobs[i].pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
        if (pid > 0)
        {
            if (WIFEXITED(status) || WIFSIGNALED(status))
            {
                // Процесс завершился
                printf("[%d] Done\t\t%s\n", jobs[i].job_id, jobs[i].command);

                // Удаляем задание из списка
                for (int j = i; j < job_count - 1; j++)
                {
                    jobs[j] = jobs[j + 1];
                }
                job_count--;
                i--;
            }
            else if (WIFSTOPPED(status))
            {
                jobs[i].status = JOB_STOPPED;
            }
            else if (WIFCONTINUED(status))
            {
                jobs[i].status = JOB_RUNNING;
            }
        }
    }
}

void set_foreground_pid(pid_t pid)
{
    foreground_pid = pid;
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

int find_latest_stopped_job()
{
    for (int i = job_count - 1; i >= 0; i--)
    {
        if (jobs[i].status == JOB_STOPPED)
        {
            return i;
        }
    }
    return -1;
}

void handle_sigtstp(int sig)
{
    if (foreground_pid > 0)
    {
        if (kill(foreground_pid, SIGTSTP) == 0)
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
    }
}

void handle_sigint(int sig)
{
    if (foreground_pid > 0)
    {
        if (kill(foreground_pid, SIGINT) == 0)
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
    check_jobs();
}

void cleanup_jobs()
{
    printf("Terminating background jobs...\n");

    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].pid > 0)
        {
            kill(jobs[i].pid, SIGTERM);
        }
    }

    // sleep(1);

    for (int i = 0; i < job_count; i++)
    {
        if (jobs[i].pid > 0)
        {
            int status;
            pid_t result = waitpid(jobs[i].pid, &status, WNOHANG);
            if (result == 0)
            {
                kill(jobs[i].pid, SIGKILL);
                waitpid(jobs[i].pid, &status, 0);
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

        printf("[%d] %s\t\t%s (pid %d)\n",
               jobs[i].job_id,
               status_str,
               jobs[i].command,
               jobs[i].pid);
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
        job_index = find_latest_stopped_job();
        if (job_index == -1)
        {
            printf("fg: no job to foreground\n");
            return;
        }
    }

    if (job_index < 0 || job_index >= job_count)
    {
        printf("fg: internal error - invalid job index\n");
        return;
    }

    pid_t pid = jobs[job_index].pid;
    if (kill(pid, SIGCONT) == 0)
    {
        jobs[job_index].status = JOB_RUNNING;
        set_foreground_pid(pid);
        printf("%s\n", jobs[job_index].command);

        int status;
        waitpid(pid, &status, WUNTRACED);

        if (WIFSTOPPED(status))
        {
            jobs[job_index].status = JOB_STOPPED;
        }
        else if (WIFEXITED(status) || WIFSIGNALED(status))
        {
            for (int j = job_index; j < job_count - 1; j++)
            {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
        }

        set_foreground_pid(0);
    }
    else
    {
        perror("fg: failed to continue job");
    }
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
        job_index = find_latest_stopped_job();
        if (job_index == -1)
        {
            printf("bg: no job to background\n");
            return;
        }
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

    pid_t pid = jobs[job_index].pid;
    if (kill(pid, SIGCONT) == 0)
    {
        jobs[job_index].status = JOB_RUNNING;
        printf("[%d] %s &\n", jobs[job_index].job_id, jobs[job_index].command);
    }
    else
    {
        perror("bg: failed to continue job");
    }
}