#pragma once

#include <stdbool.h>
#include <termios.h>

typedef struct {
    struct termios original;
    bool active;
} terminal_mode_t;

int terminal_mode_enable(terminal_mode_t *mode);
void terminal_mode_disable(terminal_mode_t *mode);