#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

volatile sig_atomic_t count = 0;

void handle_sigint(int sig) {
    write(STDOUT_FILENO, "\a", 1);
    count++;
}

void handle_sigquit(int sig) {
    printf("\nЗвуковой сигнал прозвучал %d раз(а)\n", count);
    exit(0);
}

int main() {
    signal(SIGINT, handle_sigint);
    signal(SIGQUIT, handle_sigquit);

    while(1) {
        pause();
    }

    return 0;
}
