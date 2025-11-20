#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

void terminal_restore(struct termios* old_termios) {
    if (tcsetattr(STDIN_FILENO, TCSANOW, old_termios) != 0) {
        perror("tcsetattr - restore");
        exit(1);
    }
}

int main() {
    struct termios old_termios, new_termios;

    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "Standard input is not a terminal.\n");
        return 1;
    }

    if (tcgetattr(STDIN_FILENO, &old_termios) == -1) {
        perror("tcgetattr");
        return 1;
    }

    new_termios = old_termios;
    new_termios.c_lflag &= ~(ICANON);
    new_termios.c_cc[VMIN] = 1;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) == -1) {
        perror("tcsetattr");
        return 1;
    }

    printf("Type characters: ");
    fflush(stdout);

    char ch;
    if (read(STDIN_FILENO, &ch, 1) == -1) {
        perror("read");
        terminal_restore(&old_termios);
        return 1;
    }

    terminal_restore(&old_termios);

    printf("\nYou typed: %c\n", ch);
    return 0;
}