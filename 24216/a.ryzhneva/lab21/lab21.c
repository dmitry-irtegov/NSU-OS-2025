#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

volatile sig_atomic_t count = 0;

void handle_sigint(int sig) {
    (void)sig;
    count++;
    write(STDOUT_FILENO, "\a", 1); 
}

void handle_sigquit(int sig) {
    (void)sig;

    printf("\n count of Ctrl+C = %d .\n", count);
    exit(EXIT_SUCCESS);
}

int main() {
    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror("Failed to install the SIGINT handle");
        return 1;
    }

    if (signal(SIGQUIT, handle_sigquit) == SIG_ERR) {
        perror("Failed to install the SIGQUIT handle");
        return 1;
    }

    while (1) {
        pause();
    }

    return 0;
}