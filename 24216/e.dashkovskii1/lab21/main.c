#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define SIGBELL "\a"

volatile sig_atomic_t count = 0;
volatile sig_atomic_t quit_flag = 0;

void handle_sigint(int sig) {
    (void)sig;
    count++;
    write(STDOUT_FILENO, SIGBELL, 1);
}

void handle_sigquit(int sig) {
    (void)sig;
    quit_flag = 1;
}

int main() {
    struct sigaction sa_int, sa_quit;

    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;

    sa_quit.sa_handler = handle_sigquit;
    sigemptyset(&sa_quit.sa_mask);
    sa_quit.sa_flags = SA_RESTART;

    sigaction(SIGINT, &sa_int, NULL);
    sigaction(SIGQUIT, &sa_quit, NULL);

    printf("Program started. Ctrl+C to beep, Ctrl+\\ to quit.\n");

    while (!quit_flag) {
        pause();
    }

    printf("\ncount of Ctrl+C = %d\n", count);
    return 0;
}
