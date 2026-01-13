#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ulimit.h>   
#include <sys/types.h>
#include <sys/resource.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

extern char **environ;

int main(int argc, char *argv[]) {
    int c;
    long limit_val;
    struct rlimit rlim;
    char cwd[PATH_MAX];
    char **env;
    long new_ulimit;

    while ((c = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        
        switch (c) {
        case 'i':
            printf("[Option -i]\n");
            printf("  Real UID: %ld, Effective UID: %ld\n", (long)getuid(), (long)geteuid());
            printf("  Real GID: %ld, Effective GID: %ld\n", (long)getgid(), (long)getegid());
            break;

        case 's':
            printf("[Option -s] becoming a group leader...\n");
            if (setpgid(0, 0) < 0) {
                perror("  Error setpgid");
            } else {
                printf("  Success. New PGID: %ld\n", (long)getpgrp());
            }
            break;

        case 'p':
            printf("[Option -p]\n");
            printf("  PID: %ld\n", (long)getpid());
            printf("  PPID: %ld\n", (long)getppid());
            printf("  PGID: %ld\n", (long)getpgrp());
            break;

        case 'u':
            printf("[Option -u]\n");
            limit_val = ulimit(UL_GETFSIZE);
            if (limit_val == -1) {
                perror("  receiving error ulimit");
            } else {
                printf("  ulimit (file size blocks): %ld\n", limit_val);
            }
            break;

        case 'U':
            printf("[Option -U] Setting ulimit to %s...\n", optarg);
            
            errno = 0;
            new_ulimit = strtol(optarg, NULL, 10);
            if (errno != 0) {
                perror("  Incorrect number for -U");
                break;
            }
            
            if (ulimit(UL_SETFSIZE, new_ulimit) == -1) {
                perror("  receiving error ulimit");
            } else {
                printf("  ulimit changed.\n");
            }
            break;

        case 'c':
            printf("[Option -c]\n");
            if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
                if (rlim.rlim_cur == RLIM_INFINITY)
                    printf("  Core file size: unlimited\n");
                else
                    printf("  Core file size: %ld bytes\n", (long)rlim.rlim_cur);
            } else {
                perror("  error getrlimit");
            }
            break;

        case 'C':
            printf("[Option -C] Setting core size to %s...\n", optarg);
            
            errno = 0;
            limit_val = strtol(optarg, NULL, 10);
            if (errno != 0) {
                perror("  Incorrect number for -C");
                break;
            }

            if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
                rlim.rlim_cur = (rlim_t)limit_val;
                if (setrlimit(RLIMIT_CORE, &rlim) < 0) {
                    perror("  error setrlimit");
                } else {
                    printf("  Core limit changed.\n");
                }
            }
            break;

        case 'd':
            printf("[Option -d]\n");
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("  CWD: %s\n", cwd);
            } else {
                perror("  error getcwd");
            }
            break;

        case 'v':
            printf("[Option -v] Environment variables:\n");
            for (env = environ; *env != 0; env++) {
                printf("  %s\n", *env);
            }
            break;

        case 'V':
            printf("[Option -V] Adding a variable %s...\n", optarg);
            if (putenv(optarg) != 0) {
                perror("  Error putenv");
            }
            break;

        case '?': 
            exit(1);
            
        default:
            fprintf(stderr, "An unexpected situation in getopt\n");
        }
    }

    return 0;
}
