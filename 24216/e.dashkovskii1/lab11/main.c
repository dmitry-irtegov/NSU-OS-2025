#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern char **environ;

int execvpe(const char *file, char *const argv[], char *const envp[]) {
    char **save_env = environ;
    environ = (char **)envp;
    execvp(file, argv);
    environ = save_env;
    return -1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *extra_path = "PATH=/home/students/24200/e.dashkovskii1/NSU-OS-2025/24216/e.dashkovskii1/lab11:";
    const char *old_path = getenv("PATH");

    if (!old_path) {
        old_path = "/bin:/usr/bin";
    }

    size_t newlen = strlen(extra_path) + strlen(old_path) + 1;
    char *newPath = malloc(newlen);
    if (!newPath) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    strcpy(newPath, extra_path);
    strcat(newPath, old_path);

    char *new_environ[] = { newPath, NULL };

    execvpe(argv[1], &argv[1], new_environ);

    perror("execvpe failed");
    free(newPath);
    exit(EXIT_FAILURE);
}
