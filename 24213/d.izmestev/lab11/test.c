#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int execvpe(const char *file, char *const argv[], char *const envp[]);

extern char *tzname[];

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program_to_execute>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *envp[] = {
        "TZ=America/Los_Angeles",
        "PATH=/bin:/usr/bin",
        NULL
    };

    printf("=== Testing execvpe function ===\n");
        
    if (execvpe(argv[1], &argv[1], envp) == -1) {
        perror("execvpe failed");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "execvpe returned unexpectedly\n");
    exit(EXIT_FAILURE);
}
