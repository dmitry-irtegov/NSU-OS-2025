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

#define IGNORE_ERROR_MESSAGE "failed to ignore SIGINT"
#define OUTPUT_ERROR_MESSAGE "failed to output beep"
#define RESTORE_ERROR_MESSAGE "failed to restore SIGINT handler"

void sigcatch_int (int sig) {
    if(signal(sig, SIG_IGN) == SIG_ERR) {
        write(2, IGNORE_ERROR_MESSAGE, sizeof(IGNORE_ERROR_MESSAGE));
        exit(EXIT_FAILURE);
    }
    write(1, "\07", sizeof(char));

    beep_count++;

    if (signal(sig, sigcatch_int) == SIG_ERR) {
        write(2, RESTORE_ERROR_MESSAGE, sizeof(RESTORE_ERROR_MESSAGE));
        exit(EXIT_FAILURE);
    }
}

void sigcatch_quit (int sig) {
    if(signal(sig, SIG_IGN) == SIG_ERR) {
        write(2, IGNORE_ERROR_MESSAGE, sizeof(IGNORE_ERROR_MESSAGE));
        exit(EXIT_FAILURE);
    }
    char buf[20];
    int printed;
    if((printed = snprintf(buf, 20, "\nbeep count: %d\n", beep_count)) < 0) {
        exit(EXIT_FAILURE);
    }
    write(1, buf, printed);
    exit(EXIT_SUCCESS);
}