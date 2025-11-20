#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#define PATH_MAX 4096

extern char **environ;

int execvpe(const char* file, char *const argv[], char *const envp[]) {
    if (strchr(file, '/') != 0) {
        return execve(file, argv, envp);
    }

    const char *path_value = getenv("PATH");
    if (path_value == NULL) {
        path_value = "/bin:/usr/bin";
    }

    char path_buff[PATH_MAX];
    size_t len_file = strlen(file);

    int errno_ = 0;

    const char *p = path_value;

    while (*p != '\0') {
        const char *end = strchr(p, ':');
        int len = 0;

        if (end == NULL) {
            len = strlen(p);
        } else {
            len = end - p;
        }

        //директория + '/' + имя + '\0'
        if (len + 1 + len_file + 1 > sizeof(path_buff)) {
            if (end == NULL) break;
            p = end + 1;
            continue;
        }

        if (len > 0) {
            memcpy(path_buff, p, len);
            path_buff[len] = '/';
            strcpy(path_buff + len + 1, file);
        } else {
            strcpy(path_buff, file);
        }

        execve(path_buff, argv, envp);

        if (errno == EACCES) {
            errno_ = EACCES;
        } else if (errno != ENOENT && errno != ENOTDIR) {
            return -1;
        }

        if (end == NULL) break;
        p = end + 1;
    }

    if (errno_ != 0) {
        errno = errno_;
    }

    return -1;
}

int main() {
    char *args[] = {"env", NULL}; 

    char *new_envp[] = {
        "GREETING=Hello_From_Execvpe", 
        "TEST_VAR=123", 
        NULL
    };
    
    if (execvpe("env", args, new_envp) == -1) {
        perror("execvpe failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}