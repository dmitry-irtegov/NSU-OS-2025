#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define REQUIRED_ARGS 2
#define CAT_PATH "/bin/cat"

int main(int count, char** args) {

    if (count != REQUIRED_ARGS) {
        fprintf(stderr, "Need filename argument.\n");
        return -1;
    }

    pid_t child_pid = fork();
    
    if (child_pid > 0) {
        int status_info;
        waitpid(child_pid, &status_info, 0);
        printf("Parent process finished.\n");
    } 
    else if (child_pid == 0) {
        execl(CAT_PATH, "show", args[1], NULL);
        perror("Failed to run command. Could not exec.");
        return -1;
    } 
    else {
        perror("Failed to create process.");
        return -1;
    }
    
    return 0;
}
