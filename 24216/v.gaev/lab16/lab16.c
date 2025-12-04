#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

int main() {
    struct termios old_settings, new_settings;
    char c;

    if (tcgetattr(STDIN_FILENO, &old_settings) != 0) {
        perror("Ошибка при получении атрибутов");
        return 1;
    }

    new_settings = old_settings;
    new_settings.c_lflag &= (~ICANON);
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_settings) != 0) {
        perror("Ошибка при установке атрибутов");
        return 1;
    }

    printf("2 + 2 = ");
    fflush(stdout);

    c = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);

    printf("\n\nВы нажали: '%c'\n", c);

    return 0;
}
