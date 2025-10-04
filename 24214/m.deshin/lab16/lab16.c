#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

int main() {
    struct termios old_term, new_term;

    if (tcgetattr(STDIN_FILENO, &old_term) == -1) {
        perror("failed tcgetattr");
        return 1;
    }

    new_term = old_term;
    new_term.c_lflag &= ~ICANON;
    new_term.c_cc[VMIN] = 1;
    new_term.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_term) == -1) {
        perror("failed tcsetattr");
        return 1;
    }

    char answer;
    printf("Will you pass Lab 16? (y/n): ");
    answer = getchar();
    printf("\nYour answer: %c\n", answer);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &old_term) == -1) {
        perror("failed tcsetattr");
        return 1;
    }

    return 0;
}