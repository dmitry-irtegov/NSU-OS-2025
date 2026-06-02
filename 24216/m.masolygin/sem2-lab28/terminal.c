#include "terminal.h"

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void terminal_restore(void) { tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios); }

void terminal_init(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(terminal_restore);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
}
