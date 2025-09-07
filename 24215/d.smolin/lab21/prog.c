#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>

volatile sig_atomic_t count = 0;
volatile sig_atomic_t flag = 0;

void sigint(int sig) {
    signal(sig, SIG_IGN);
    
    count++;
    write(STDOUT_FILENO, "\a", 1);
    
    signal(sig, sigint);
}

void sigquit(int sig) {
    signal(sig, SIG_IGN);
    
    flag = 1;
    
    signal(sig, sigquit);
}

int main() {
    struct termios old, new;
    tcgetattr(STDIN_FILENO, &old);
    new = old;
    new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new);

    signal(SIGINT, sigint);
    signal(SIGQUIT, sigquit);

    while (!flag) {
        pause();
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old);

    printf("\nНасчитано Ctrl-C: %d\n", (int)count);
    return 0;
}