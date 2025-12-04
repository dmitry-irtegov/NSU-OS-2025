#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

int main(void) {
    struct termios old_settings, new_settings;
    char answer;

    if (tcgetattr(STDIN_FILENO, &old_settings) != 0) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }

    new_settings = old_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_settings) != 0) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }

    printf("2 + 2 = ?");
    fflush(stdout);

    answer = getchar();
    printf("\n\nВы нажали: '%c'\n", answer);
    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);

    return 0;
}
