#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern char **environ;

int execvpe(const char *file, char *const argv[], char *const envp[]) {
    if (file == NULL || argv == NULL || envp == NULL || file[0] == '\0' || argv[0] == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (strchr(file, '/') != NULL) {
        return execve(file, argv, envp);
    }

    char **old_environ = environ;
    environ = (char **)envp;
    int result = execvp(file, argv);

    environ = old_environ;
    return result;
}
