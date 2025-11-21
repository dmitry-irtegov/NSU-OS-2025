#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
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
        exit(EXIT_FAILURE);
    }

    char *new_env[] = {
        "MYVAR=HelloWorld",
        NULL
    };

    execvpe(argv[1], &argv[1], new_env);
    perror("Error: execvpe");
    exit(EXIT_FAILURE);
}