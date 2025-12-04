#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <string.h>
#include <sys/resource.h>

extern char **environ;

void print_ulimit() 
{
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) //на количество файлов в качестве примера 
    {
        printf("ulimit: soft=%llu, hard=%llu\n", rl.rlim_cur, rl.rlim_max);
    }
    else
    {
        perror("getrlimit");
    }
}

void set_ulimit(const char *value) 
{
    long new_ulimit = atol(value);
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) 
    {
        rl.rlim_cur = new_ulimit;
        if (setrlimit(RLIMIT_NOFILE, &rl) == -1) 
        {
            perror("setrlimit");
        }
    } 
    else 
    {
        perror("getrlimit");
    }
}

void print_core_size() 
{
    struct rlimit rl;
    if (getrlimit(RLIMIT_CORE, &rl) == 0)
    {
        printf("Core file size limit: %llu\n", rl.rlim_cur);
    }
    else
    {
        perror("getrlimit");
    }
}

void set_core_size(const char *value) 
{
    long size = atol(value);
    struct rlimit rl;
    rl.rlim_cur = size;
    rl.rlim_max = size;
    if (setrlimit(RLIMIT_CORE, &rl) == -1) 
    {
        perror("setrlimit");
    }
}

void print_cwd() 
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
    {
        printf("Current working directory: %s\n", cwd);
    }
    else
    {
        perror("getcwd");
    }
}

void set_env_var(const char *arg) 
{
    char *eq = strchr(arg, '=');
    if (eq) 
    {
        *eq = '\0';
        if (setenv(arg, eq + 1, 1) == -1)
        {
            perror("setenv");
        }
    } 
    else 
    {
        fprintf(stderr, "Invalid format for -V, use NAME=VALUE\n");
    }
}

int main(int argc, char* argv[]) 
{
    int opt;
    
    while ((opt = getopt(argc, argv, "ispudvU:C:V:")) != -1) {
        switch (opt) {
            case 'i':
                printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());
                printf("Real GID: %d, Effective GID: %d\n", getgid(), getegid());
                break;
            case 's':
                if (setpgid(0, 0) == -1)
                {
                    perror("setpgid");
                }
                break;
            case 'p':
                printf("PID: %d, PPID: %d, PGID: %d\n", getpid(), getppid(), getpgrp());
                break;
            case 'u':
                print_ulimit();
                break;
            case 'U':
                if (optarg) 
                {
                    set_ulimit(optarg);
                }
                break;
            case 'c':
                print_core_size();
                break;
            case 'C':
                if (optarg) 
                {
                    set_core_size(optarg);
                }
                break;
            case 'd':
                print_cwd();
                break;
            case 'v':
                for (char **env = environ; *env != NULL; env++)
                { 
                    printf("%s\n", *env);
                }
                break;
            case 'V':
                if (optarg) 
                {
                    set_env_var(optarg);
                }
                break;
            default:
                fprintf(stderr, "Usage: %s [-i] [-s] [-p] [-u] [-U value] [-c] [-C value] [-d] [-v] [-V name=value]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    return 0;
}