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
    time_t now;
    (void)time(&now);
    if (argc < 2) {
        fprintf(stderr, "No attribute file\n");
        exit(-1);
    }

    char* path = getenv("PATH");
    char* my_env = "PATH=/home/students/24200/o.kataeva/NSU-OS-2025/24214/o.kataeva/lab11/:";
    char newPath[strlen(path) + strlen(my_env) + 2];
    strcpy(newPath, my_env);
    strcat(newPath, path);

    printf("Time Barnaul: %s", ctime(&now));
    
    char *new_env[] = {newPath, "TZ=America/Los_Angeles", NULL};

    execvpe(argv[1], &argv[1], new_env);
    perror("FAIL: execvpe");
    exit(-1);
}
