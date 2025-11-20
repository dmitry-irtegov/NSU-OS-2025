#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define BEEP "\a"

volatile sig_atomic_t counter = 0;
volatile sig_atomic_t exit_flag = 0;

void beep(int sig) {
    write(STDOUT_FILENO, BEEP, sizeof(BEEP) - 1);
    counter++;
}

void end(int sig) { exit_flag = 1; }

void setup_signals(struct sigaction* sa, void (*handler)(int), int signum) {
    memset(sa, 0, sizeof(*sa));
    sa->sa_handler = handler;
    if (sigemptyset(&sa->sa_mask) == -1) {
        perror("sigemptyset");
        exit(1);
    }
    sa->sa_flags = 0;
    if (sigaction(signum, sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
}

int main() {
    struct sigaction sa_beep;
    setup_signals(&sa_beep, beep, SIGINT);

    struct sigaction sa_end;
    setup_signals(&sa_end, end, SIGQUIT);

    printf("Press Ctrl+C to beep, Ctrl+\\ to quit.\n");
    while (!exit_flag) {
        pause();
    }
    printf("\nSignal received %d times\n", counter);
    return 0;
}