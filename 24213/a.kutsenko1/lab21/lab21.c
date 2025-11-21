#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int bcounter = 0;

void sigint_handl(int sig) {
    write(STDOUT_FILENO, "\a", 1);
    bcounter++;
}

void sigquit_handl(int sig) {
    char msg[50];
    int len = sprintf(msg, "\nКонец. Было звуков: %d\n", bcounter);
    write(STDOUT_FILENO, msg, len);
    _exit(0);
}

int main() {
    if (signal(SIGINT, sigint_handl) == SIG_ERR) {
        perror("Error setting SIGINT handler");
        return 1;
    }
    
    if (signal(SIGQUIT, sigquit_handl) == SIG_ERR) {
        perror("Error setting SIGQUIT handler");
        return 1;
    }
    printf("Нужно нажать Ctrl+C для звукового сигнала.\n");
    printf("Нужно нажать Ctrl+\\ для вывода количества сигналов и завершения программы.\n");
    while(1) {
        pause();
    }
    return 0;
}