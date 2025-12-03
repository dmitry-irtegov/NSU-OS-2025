#include "io.h"

#include <stdarg.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

static struct termios term_attr;

int check_terminal() {
    if (!isatty(0)) {
        fprintf(stderr, "Only terminal input is supported.\n");
        return 1;
    }
    return 0;
}

int save_terminal() {
    if (tcgetattr(0, &term_attr)) {
        perror("Could not save terminal attributes");
        return 1;
    }
    return 0;
}

int restore_terminal() {
    if (tcsetattr(0, TCSAFLUSH, &term_attr)) {
        perror("Could not restore terminal attributes");
        return 1;
    }
    return 0;
}

int set_foreground(pid_t pgid) {
    if (tcsetpgrp(0, pgid)) {
        perror("Could not set foreground process group");
        return 1;
    }
    return 0;
}
