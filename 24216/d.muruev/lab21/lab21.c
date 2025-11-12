#include <stdio.h>
#include <stdlib.h>
#include <signal.h> 
#include <unistd.h> 

#define SIGINT_ERR "Ошибка установки обработчика SIGINT"
#define SIGQUIT_ERR "Ошибка установки обработчика SIGQUIT"

volatile sig_atomic_t beep_count = 0;
volatile sig_atomic_t quit_flag = 0;


void handle_sigint(int sig) {
    beep_count++;
    
    write(STDOUT_FILENO, "\a", 1);

    if (signal(sig, handle_sigint) == SIG_ERR) {
        write(STDERR_FILENO, SIGINT_ERR, sizeof(SIGINT_ERR));
        exit(EXIT_FAILURE);
    }
}


void handle_sigquit(int sig) {
    quit_flag = 1;

    if (signal(sig, handle_sigquit) == SIG_ERR) {
        write(STDERR_FILENO, SIGQUIT_ERR, sizeof(SIGQUIT_ERR));
        exit(EXIT_FAILURE);
    }
}

int main() {

    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror(SIGINT_ERR);
        exit(EXIT_FAILURE);
    }

    if (signal(SIGQUIT, handle_sigquit) == SIG_ERR) {
        perror(SIGQUIT_ERR);
        exit(EXIT_FAILURE);
    }

    while (quit_flag == 0) {
        pause();
    }

    printf("Всего прозвучало сигналов: %d\n", (int)beep_count);

    return 0;
}