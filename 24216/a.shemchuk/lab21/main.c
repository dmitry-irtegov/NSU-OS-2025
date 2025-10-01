#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

volatile sig_atomic_t beep_count;

void sigcatch_int(int sig);
void sigcatch_quit(int sig);

int main(void) {
    if(signal(SIGINT, sigcatch_int) == SIG_ERR) {
        perror("main: failed to set SIGINT handler");
        exit(EXIT_FAILURE);
    }

    if(signal(SIGQUIT, sigcatch_quit) == SIG_ERR) {
        perror("main: failed to set SIGQUIT handler");
        exit(EXIT_FAILURE);
    }

    setbuf(stdout, (char *) NULL);

    while (1) {
        pause();
    }
    return 0;
}

void sigcatch_int (int sig) {
    if(signal(sig, SIG_IGN) == SIG_ERR) {
        perror("sigcatch_int: failed to ignore SIGINT");
        return;
    }
    if(putchar('\07') == EOF) {
        perror("sigcatch_int: failed to output beep");
    }

    beep_count++;

    if (signal(sig, sigcatch_int) == SIG_ERR) {
        perror("sigcatch_int: failed to restore SIGINT handler");
    }
}

void sigcatch_quit (int sig) {
    if(signal(sig, SIG_IGN) == SIG_ERR) {
        perror("sigcatch_quit: failed to ignore SIGQUIT");
    }
    printf("\nbeep count: %d\n", beep_count);
    exit(EXIT_SUCCESS);
}