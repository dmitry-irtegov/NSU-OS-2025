#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

#define FILE_PATH "./file.txt"

int main() {
    pid_t pid = fork();
    int status;

    switch(pid) {
        case -1:
            perror("Fork failed.");
            return 1;
        case 0:
            execl("/bin/cat", "cat", FILE_PATH, NULL); 
            perror("Execl failed.");
            return 1;
        default:
            printf("Parent: waiting for child.\n");
            if (waitpid(pid, &status, 0) == -1) {
               perror("Waitpid failed.");
               return 1;
            }
            printf("Parent: child exited.\n");
            return 0;
    }
}
