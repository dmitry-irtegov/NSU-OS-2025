#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

int read_char_no_enter(char *out)
{
    struct termios told, tnew;

    if (tcgetattr(STDIN_FILENO, &told) == -1) {
        return -1;
    }

    tnew = told;

    tnew.c_lflag &= ~ICANON;
    tnew.c_cc[VMIN]  = 1;
    tnew.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &tnew) == -1) {
        return -1;
    }

    ssize_t nread = read(STDIN_FILENO, out, 1);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &told) == -1) {
        return -1;
    }

    return (nread == 1) ? 0 : -1;
}

int main(void)
{
    char answer;

    printf("Продолжить выполнение? (y/n): ");
    fflush(stdout);

    if (read_char_no_enter(&answer) == -1) {
        perror("Ошибка ввода");
        exit(EXIT_FAILURE);
    }

    if (answer == 'y' || answer == 'Y') {
        printf("\nОк, продолжаем...\n");
    } else {
        printf("\nЗавершаем программу.\n");
    }

    return 0;
}
