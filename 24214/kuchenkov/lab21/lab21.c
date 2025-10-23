#include <stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

int count = 0;

void signal_handler(int signal_number) {
    signal(signal_number, signal_handler);
    if (signal_number == SIGINT) {
        write(STDOUT_FILENO, "\a", 1);
        count++;
    } else if (signal_number == SIGQUIT) {
        char buffer[32];
        const char prefix[] = " signals were made\n";
        int i = sizeof(buffer) - 1;
        buffer[i] = '\0';

        int num = count;
        if (num == 0) {
            buffer[--i] = '0';
        } else {
            while (num > 0 && i > 0) {
                buffer[--i] = '0' + (num % 10);
                num /= 10;
            }
        }
        
        write(STDOUT_FILENO, &buffer[i], strlen(&buffer[i]));
        write(STDOUT_FILENO, prefix, sizeof(prefix) - 1);
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