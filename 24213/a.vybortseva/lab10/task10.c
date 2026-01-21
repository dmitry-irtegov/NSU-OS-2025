#include <unistd.h>
#include <wait.h>
#include <sys/types.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "not enough arguments\n");
        return -1;
    }

    pid_t pid;
    if ((pid = fork()) > 0) {  //p
        int wstatus;
        if (wait(&wstatus) == -1) {
            perror("waitpid error");
            return -1;
        }

        if (!WIFEXITED(wstatus)) {
            fprintf(stderr, "the child did not terminate normally\n");
            return -1;
        }

        printf("exit status: %d\n", WEXITSTATUS(wstatus));
        return 0;

    } else if (pid == 0) {  //ch
        execvp(argv[1], argv + 1);
        perror("error execvp");
        return -1;
    } else {
        perror("fork error");
        return -1;
    }
}
