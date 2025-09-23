#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern char **environ;

int execvpe(const char* filename, char** argv, char* const envp[]) {
    char** tmp_environ = environ;
    environ = (char**)envp;
    execvp(filename, argv);
    environ = tmp_environ;
    return -1;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "No attribute file\n");
        exit(-1);
    }

    char* path = getenv("PATH");
    char* my_env = "PATH=/home/students/24200/o.kataeva/NSU-OS-2025/24213/o.kataeva/lab11/:";
    char newPath[strlen(path) + strlen(my_env) + 2];
    strcpy(newPath, my_env);
    strcat(newPath, path);

    char *new_env[] = {
        newPath, NULL};

    execvpe(argv[1], &argv[1], new_env);
    perror("FAIL: execvpe");
    exit(-1);
}
