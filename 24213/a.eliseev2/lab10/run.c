#include <siginfo.h>
#include <stdio.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "You need to specify at least one argument "
                        "containing the name of the program to run.\n");
        return 1;
    }

    pid_t result = fork();

    if (result < 0) {
        perror("fork failed");
        return 1;
    } else if (result == 0) {
        if (execvp(argv[1], argv + 1) == -1) {
            perror("execv failed");
            return 1;
        }
    }

    siginfo_t info;
    if (waitid(P_PID, result, &info, WEXITED)) {
        perror("waitid failed");
        return 1;
    }

    int code = info.si_status;

    printf("Child process exited with code %d.\n", code);
    fflush(stdout);

    return 0;
}
