#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

int execvpe(const char *file, char *const argv[], char *const envp[]) {
    if (file == NULL || argv == NULL || envp == NULL) {
        errno = EINVAL;
        return -1;
    }

    for (char *const *env = envp; *env != NULL; ++env) {
        char *equal_sign = strchr(*env, '=');
        if (equal_sign == NULL || equal_sign == *env) {
            errno = EINVAL;
            return -1;
        }
    }

    char **old_environ = environ;
    environ = (char **)envp;

    int result = execvp(file, argv);
    int saved_errno = errno;

    environ = old_environ;
    errno = saved_errno;

    return result;
}

int main(void) {
    char *args[] = {"env", NULL};
    char *new_envp[] = {"PA=", NULL};

    pid_t pid = fork();
    if (pid < 0) {
        perror("Failed to fork");
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        execvpe("env", args, new_envp);
        perror("Failed to execute execvpe");
        _exit(EXIT_FAILURE);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        perror("Failed to wait for the child process");
        return EXIT_FAILURE;
    }

    if (WIFEXITED(status)) {
        printf("\nChild process exited with code %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("\nChild process exited due to signal %d\n", WTERMSIG(status));
    } else {
        printf("\nChild process exited abnormally\n");
    }

    return EXIT_SUCCESS;
}