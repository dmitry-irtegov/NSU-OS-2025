#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>

volatile sig_atomic_t beep_count = 0;
volatile sig_atomic_t quit_flag = 0;

void sigint_handler(int sig) {
    beep_count++;
    char beep = '\a';
    write(STDOUT_FILENO, &beep, 1);
    signal(SIGINT, sigint_handler);
}

void sigquit_handler(int sig) {
    quit_flag = 1;
}

int main() {
    signal(SIGINT, sigint_handler);
    signal(SIGQUIT, sigquit_handler);

    struct termios oldt, newt;
    if (tcgetattr(STDIN_FILENO, &oldt) == -1) {
        perror("tcgetattr");
        exit(1);
    }
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &newt) == -1) {
        perror("tcsetattr");
        exit(1);
    }

    printf("Press Ctrl-C to beep. Press Ctrl-\\ to quit.\n");
    fflush(stdout);

    while (!quit_flag) {
        pause();
    }

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldt) == -1) {
        perror("tcsetattr restore");
        exit(1);
    }

    printf("\nSignal SIGQUIT received. Beep count: %d\n", beep_count);
    return 0;
}
