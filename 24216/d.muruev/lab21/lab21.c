#include <stdio.h>
#include <stdlib.h>
#include <signal.h> 
#include <unistd.h> 

#define SIGINT_ERR "Ошибка установки обработчика SIGINT"

volatile sig_atomic_t beep_count = 0;
volatile sig_atomic_t quit_flag = 0;


void handle_sigint(int sig) {
    beep_count++;
    
    write(STDOUT_FILENO, "\a", 1);

    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        write(STDERR_FILENO, SIGINT_ERR, sizeof(SIGINT_ERR));
        exit(EXIT_FAILURE);
    }
}


void handle_sigquit(int sig) {
    quit_flag = 1;
}

int main() {

    if (signal(SIGINT, handle_sigint) == SIG_ERR) {
        perror(SIGINT_ERR);
        exit(EXIT_FAILURE);
    }

    if (signal(SIGQUIT, handle_sigquit) == SIG_ERR) {
        perror("Ошибка установки обработчика SIGQUIT");
        exit(EXIT_FAILURE);
    }

    printf("Нажмите Ctrl+C для звукового сигнала.\n");
    printf("Нажмите Ctrl+\\ для выхода.\n");


    while (quit_flag == 0) {
        pause();
    }

    
    printf("\nПолучен сигнал SIGQUIT. Завершение.\n");

    printf("Всего прозвучало сигналов: %d\n", (int)beep_count);

    return 0;
}