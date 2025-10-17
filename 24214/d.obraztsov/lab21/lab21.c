#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t beep_count = 0;

void signal_handler(int sig){
    switch(sig){
    case SIGINT:
        beep_count++;
        write(1, "\a", 1);
        break;
    case SIGQUIT:
        write(1, "\n", 1);
        char count_str[20];
        int len = 0;
        int n = beep_count;
        
        if (n == 0) {
            count_str[len++] = '0';
        } else {
            char temp[20];
            int i = 0;
            while (n > 0) {
                temp[i++] = '0' + (n % 10);
                n /= 10;
            }
            while (i > 0) {
                count_str[len++] = temp[--i];
            }
        }
        count_str[len++] = '\n';
        write(1, count_str, len);
        _exit(0);
        break;
    }
}

int main() {
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        exit(1);
    }
    
    if (signal(SIGQUIT, signal_handler) == SIG_ERR) {
        exit(1);
    }
    
    while (1) {
        pause();
    }
    
    return 0;
}