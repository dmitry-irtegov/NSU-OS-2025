#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

volatile int count = 0;

void handle_sigint(int sig) {
    (void)sig;
    write(STDOUT_FILENO, "\a", 1);
    count++;
}

void handle_sigquit(int sig) {
    (void)sig;
    printf("\nЗвуковой сигнал прозвучал %d раз(а)\n", count);
    exit(0);
}

int main() {
    struct sigaction sa;

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = handle_sigquit;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGQUIT, &sa, NULL);

    while (1) {
        pause();
    }

    return 0;
}
