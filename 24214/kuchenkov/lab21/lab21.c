#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int count = 0;

void signal_handler(int signal_number) {
    if (signal_number == SIGINT) {
        printf("\a");
        fflush(stdout);
        count++;
    } else if (signal_number == SIGQUIT) {
        printf("Сделано %d сигналов", count);
        exit(0);
    }
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);

    while (1) {
        pause();
    }

    return 0;
}