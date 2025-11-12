#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int main(void) {
    struct termios oldt, newt;
    int ch;

    if (tcgetattr(STDIN_FILENO, &oldt) == -1) {
        perror("tcgetattr");
        return 1;
    }

    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == -1) {
        perror("tcsetattr");
        return 1;
    }

    printf("Введите символ: ");
    fflush(stdout);

    ch = getchar();

    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1) {
        perror("tcsetattr");
        return 1;
    }

    printf("\nВы ввели: %c\n", ch);

    return 0;
}
