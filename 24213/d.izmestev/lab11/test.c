#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int execvpe(const char *file, char *const argv[], char *const envp[]);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s [ENV_VAR=VALUE...] -- <program> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int separator_pos = -1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) {
            separator_pos = i;
            break;
        }
    }

    if (separator_pos == -1 || separator_pos == argc - 1) {
        fprintf(stderr, "Error: Missing separator '--' before program name\n");
        fprintf(stderr, "Usage: %s [ENV_VAR=VALUE...] -- <program> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int env_count = separator_pos - 1;
    char **myenvp = malloc((env_count + 2) * sizeof(char *));
    if (!myenvp) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < env_count; i++) {
        myenvp[i] = argv[i + 1];
    }
    myenvp[env_count] = NULL;

    int prog_argc = argc - separator_pos - 1;
    char **prog_argv = &argv[separator_pos + 1];

    printf("=== execvpe test ===\n");
    printf("Environment variables:\n");
    for (int i = 0; i < env_count; i++) {
        printf(" - %s\n", myenvp[i]);
    }
    printf("Executing program: %s\n", prog_argv[0]);
    printf("Program arguments:\n");
    for (int i = 0; i < prog_argc; i++) {
        printf("  [%d]: %s\n", i, prog_argv[i]);
    }
    printf("====================\n");

    if (execvpe(prog_argv[0], prog_argv, myenvp) == -1) {
        perror("execvpe failed");
        free(myenvp);
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "execvpe returned unexpectedly\n");
    free(myenvp);
    exit(EXIT_FAILURE);
}
