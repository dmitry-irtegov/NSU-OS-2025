#include <unistd.h>
#include "terminal_mode.h"

int terminal_mode_enable(terminal_mode_t *mode) {
    mode->active = false;

    if (tcgetattr(STDIN_FILENO, &mode->original) != 0) {
        return 0;
    }

    struct termios raw = mode->original;
    raw.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) {
        return -1;
    }

    mode->active = true;
    return 0;
}

void terminal_mode_disable(terminal_mode_t *mode) {
    if (mode == NULL || !mode->active) {
        return;
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &mode->original);
    mode->active = false;
}