#include "io.h"
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

int check_terminal(int fd) {
    if (!isatty(fd)) {
        fprintf(stderr, "Only terminal input is supported.\n");
        return 1;
    }
    return 0;
}

int setup_terminal(int fd, struct termios *old) {
    if (tcgetattr(fd, old)) {
        perror("Could not get terminal attributes");
        return 1;
    }
    struct termios new = *old;
    new.c_lflag |= ICANON;
    new.c_lflag &= ~ECHOCTL;
    new.c_iflag |= IMAXBEL;
    if (tcsetattr(fd, TCSAFLUSH, &new)) {
        perror("Could not set terminal attributes");
        return 1;
    }
    return 0;
}

int save_terminal(int fd, struct termios *attr) {
    if (tcgetattr(fd, attr)) {
        perror("Could not save terminal attributes");
        return 1;
    }
    return 0;
}

int restore_terminal(int fd, struct termios *attr) {
    if (tcsetattr(fd, TCSAFLUSH, attr)) {
        perror("Could not restore terminal attributes");
        return 1;
    }
    return 0;
}

int set_foreground(int fd, pid_t pgid) {
    if (tcsetpgrp(0, pgid)) {
        perror("Could not set foreground process group");
        return 1;
    }
    return 0;
}
