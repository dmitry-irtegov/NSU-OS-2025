#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

int main(void) {
    struct termios oldt, newt;

    if (tcgetattr(STDIN_FILENO, &oldt) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }

    newt = oldt;
    newt.c_lflag &= ~(ICANON);
    newt.c_cc[VMIN]  = 1;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }

    printf("Продолжить? (y/n): ");
    int ch;
    ch = getchar();
    if (ch == EOF) {
        perror("getchar");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        exit(EXIT_FAILURE);
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1) {
        perror("tcsetattr restore");
        exit(EXIT_FAILURE);
    }

    printf("\nВы ввели: %c\n", ch);
    return 0;
}
