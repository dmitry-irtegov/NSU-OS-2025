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
    char *endptr;

    if (argc == 1) {
        fprintf(stderr, "Usage: %s [-ispuU cC:dvV:]\n", argv[0]);
        return 0;
    }

    while ((c = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        switch (c) {
        case 'i':
            printf("[Option -i]\n");
            printf("RUID: %ld, EUID: %ld\n", (long)getuid(), (long)geteuid());
            printf("RGID: %ld, EGID: %ld\n", (long)getgid(), (long)getegid());
            break;

        case 's':
            printf("[Option -s]\n");
            printf("Becoming a group leader...\n");
            if (setpgid(0, 0) < 0) {
                perror("setpgid");
            } else {
                printf("Success. New PGID: %ld\n", (long)getpgrp());
            }
            break;

        case 'p':
            printf("[Option -p]\n");
            printf("PID: %ld\n", (long)getpid());
            printf("PPID: %ld\n", (long)getppid());
            printf("PGID: %ld\n", (long)getpgrp());
            break;

        case 'u':
            printf("[Option -u]\n");
            limit_val = ulimit(UL_GETFSIZE);
            if (limit_val == -1) {
                perror("ulimit");
            } else {
                printf("File size limit: %ld blocks\n", limit_val);
            }
            break;

        case 'U':
            printf("[Option -U] Setting ulimit to %s...\n", optarg);
            errno = 0;
            limit_val = strtol(optarg, &endptr, 10);
            
            if (errno != 0 || limit_val < 0 || *endptr != '\0') {
                fprintf(stderr, "Error: invalid value for -U (must be number)\n");
                break;
            }
            
            if (ulimit(UL_SETFSIZE, limit_val) == -1) {
                perror("ulimit");
            } else {
                printf("ulimit changed to %ld\n", limit_val);
            }
            break;

        case 'c':
            printf("[Option -c]\n");
            if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
                if (rlim.rlim_cur == RLIM_INFINITY)
                    printf("Core dump size: unlimited\n");
                else
                    printf("Core dump size: %ld bytes\n", (long)rlim.rlim_cur);
            } else {
                perror("getrlimit");
            }
            break;

        case 'C':
            printf("[Option -C] Setting core size to %s...\n", optarg);
            errno = 0;
            limit_val = strtol(optarg, &endptr, 10);
            
            if (errno != 0 || limit_val < 0 || *endptr != '\0') {
                fprintf(stderr, "Error: invalid value for -C (must be number)\n");
                break;
            }
            
            if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
                rlim.rlim_cur = (rlim_t)limit_val;
                rlim.rlim_max = RLIM_INFINITY;
                
                if (setrlimit(RLIMIT_CORE, &rlim) < 0) {
                    perror("setrlimit");
                } else {
                    printf("Core dump limit changed to %ld bytes\n", limit_val);
                }
            } else {
                perror("getrlimit");
            }
            break;

        case 'd':
            printf("[Option -d]\n");
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("Current directory: %s\n", cwd);
            } else {
                perror("getcwd");
            }
            break;

        case 'v':
            printf("[Option -v] Environment:\n");
            for (env = environ; *env != NULL; env++) {
                printf("%s\n", *env);
            }
            break;

        case 'V':
            printf("[Option -V] Setting %s\n", optarg);
            if (putenv(optarg) != 0) {
                perror("putenv");
            } else {
                printf("Environment variable set\n");
            }
            break;

        case '?':
            fprintf(stderr, "Unknown option: -%c\n", optopt);
            return 1;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Warning: non-option arguments ignored: ");
        for (int i = optind; i < argc; i++) {
            fprintf(stderr, "%s ", argv[i]);
        }
        fprintf(stderr, "\n");
    }

    return 0;
}
