#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

int main() {
    struct termios oldt, newt;
    char answer;
    int result;

    if (tcgetattr(STDIN_FILENO, &oldt) == -1) {
        perror("tcgetattr");
        exit(1);
    }

    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1;
    newt.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &newt) == -1) {
        perror("tcsetattr");
        exit(1);
    }

    printf("Are you sleepy? (y/n)\n");
    fflush(stdout);

    result = read(STDIN_FILENO, &answer, 1);
    if (result == -1) {
        perror("read");
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldt);
        exit(1);
    }

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldt) == -1) {
        perror("tcsetattr");
        exit(1);
    }

    switch (answer) {
    case 'y':
        printf("You need to go to sleep!\n");
        break;

    case 'n':
        printf("You can go back to your work!\n");
        break;

    default:
        printf("I don't understand you! You pressed: %c\n", answer);
        break;
    }

    return 0;
}