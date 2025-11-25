#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern char **environ;

int my_execvpe(const char *file, char *const argv[], char *const envp[]) {
    char **old_env = environ;
    environ = (char **)envp;
    execvp(file, argv);
    environ = old_env;
    return -1;
}

int find_var(char *const env[], const char *var) {
    size_t n = strlen(var);
    for (int i = 0; env && env[i]; i++) {
        if (strncmp(env[i], var, n) == 0 && env[i][n] == '=') {
            return i;
        }
    }
    return -1;
}

int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "Too little arguments!\nUsage: %s <command> [arg0...argn]\n", argv[0]);
        return 1;
    }

    const char *my_path = "PATH=/home/students/24200/m.deshin:";
    const char *old_path = getenv("PATH");

    if (!old_path) {
        old_path = "/usr/bin:/usr/sbin:/sbin:/usr/gnu/bin";
    }

    size_t new_len = strlen(my_path) + strlen(old_path) + 1;
    char *new_path = malloc(new_len);

    if (!new_path) {
        perror("malloc failed");
        return 1;
    }

    strcpy(new_path, my_path);
    strcat(new_path, old_path);

    int cnt = 0;
    while (environ[cnt]) cnt++;

    char **merged_env = malloc((cnt + 2) * sizeof(char *));
    if (!merged_env) {
        perror("malloc failed");
        free(new_path);
        return 1;
    }

    for (int i = 0; i < cnt; i++) {
        merged_env[i] = environ[i];
    }
    merged_env[cnt] = NULL;

    int idx = find_var(merged_env, "PATH");
    if (idx >= 0) {
        merged_env[idx] = new_path;
    } else {
        merged_env[cnt++] = new_path;
        merged_env[cnt] = NULL;
    }

    my_execvpe(argv[1], &argv[1], merged_env);
    perror("my_execvpe failed");

    free(merged_env);
    free(new_path);
    return 1;
}
