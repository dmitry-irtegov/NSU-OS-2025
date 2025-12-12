#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

int count = 0;

void signal_handler(int signal_number) {
    signal(signal_number, signal_handler);
    if (signal_number == SIGINT) {
        if (write(1, "\a", 1) == -1) {
            _exit(EXIT_FAILURE);
        }
        count++;
    } else if (signal_number == SIGQUIT) {
        char buffer[32];
        int len = sprintf(buffer, "%d signals were made\n", count);
        if (write(1, buffer, len) == -1) {
            _exit(EXIT_FAILURE);
        }
        _exit(0);
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