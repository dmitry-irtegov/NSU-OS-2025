#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static struct termios oldt;

static void restore_term(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldt);
}

int main(void) {
    struct termios newt;

    if (tcgetattr(STDIN_FILENO, &oldt) == -1) {
        perror("tcgetattr");
        return 1;
    }
    atexit(restore_term);

    newt = oldt;
    newt.c_lflag &= ~ICANON;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &newt) == -1) {
        perror("tcsetattr");
        return 1;
    }

    printf("Choose answer [y/n]: ");
    fflush(stdout);

    unsigned char ch;
    if (read(STDIN_FILENO, &ch, 1) <= 0) {
        return 1;
    }

    restore_term();
    printf("\nYou chose: %c\n", ch);
    return 0;
}
