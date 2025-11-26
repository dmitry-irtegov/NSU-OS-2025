#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#define SIG "\a"

volatile sig_atomic_t count = 0;
volatile sig_atomic_t quit_flag = 0;

void handle_sigint(int sig) {
    (void)sig;
    count++;
    write(STDOUT_FILENO, SIG, sizeof(SIG) - 1); 
}

void handle_sigquit(int sig) {
    (void)sig;
    quit_flag = 1;
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

    printf("Program started. Ctrl+C to beep, Ctrl+\\ to quit.\n");

    while (!quit_flag) {
        pause(); 
    }

    printf("\n count of Ctrl+C = %d.\n", count);

    return 0;
}