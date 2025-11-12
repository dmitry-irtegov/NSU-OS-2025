#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

#define NUM_SIGNALS 5

static int signals_to_handle[] = {SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU};

void signal_handler(int signum) {
    // Ignore signals in the shell
}

void ignore_signals() {
    for (int i = 0; i < NUM_SIGNALS; i++) {
        if (signal(signals_to_handle[i], SIG_IGN) == SIG_ERR) {
            perror("signal");
            exit(1);
        }
    }
}

void activate_signals() {
    for (int i = 0; i < NUM_SIGNALS; i++) {
        if (signal(signals_to_handle[i], SIG_DFL) == SIG_ERR) {
            perror("signal");
            exit(1);
        }
    }
}