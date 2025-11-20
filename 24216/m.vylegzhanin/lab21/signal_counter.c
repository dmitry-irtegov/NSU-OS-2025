#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

volatile sig_atomic_t count = 0;
volatile sig_atomic_t quit_flag = 0;

void sigcatch(int sig)
{
    switch(sig){
        case SIGQUIT:
            quit_flag = 1;
            break;
            
        case SIGINT:
            write(STDOUT_FILENO, "Beep\a\n", sizeof("Beep\n\a") - 1);
            count++;
            break;
    }
}

int main()
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigcatch;
    
    if (sigemptyset(&sa.sa_mask) == -1) {
        perror("sigemptyset");
        exit(EXIT_FAILURE);
    }

    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
        exit(EXIT_FAILURE);
    }
    
    if (sigaction(SIGQUIT, &sa, NULL) == -1) {
        perror("sigaction SIGQUIT");
        exit(EXIT_FAILURE);
    }
    
    while(1){
        pause();
        
        if (quit_flag) {
            printf("\n%d signals count\n", count);
            exit(EXIT_SUCCESS);
        }
    }
    
    return 0;
}
