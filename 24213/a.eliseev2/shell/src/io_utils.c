#include "io.h"
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

int fdprintf(int fd, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int count = vsnprintf(NULL, 0, format, args);
    va_end(args);
    char buffer[count + 1];
    va_start(args, format);
    vsprintf(buffer, format, args);
    va_end(args);
    return write(fd, buffer, count);
}

int setup_terminal(int fd, struct termios *old, struct termios *new) {
    if (!isatty(fd)) {
        fprintf(stderr, "Only terminal input is supported.\n");
        return 1;
    }
    if (tcgetattr(fd, new)) {
        perror("Could not get terminal attributes");
        return 1;
    }
    *old = *new;
    new->c_lflag |= ICANON;
    new->c_lflag &= ~ECHOCTL;
    new->c_iflag |= IMAXBEL;
    if (tcsetattr(fd, TCSAFLUSH, new)) {
        perror("Could not set terminal attributes");
        return 1;
    }
    return 0;
}

int restore_terminal(int fd, struct termios *old) {
    if (tcsetattr(fd, TCSAFLUSH, old)) {
        perror("Could not restore terminal attributes");
        return 1;
    }
    return 0;
}
