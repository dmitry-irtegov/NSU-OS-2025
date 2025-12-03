#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>


void set_terminal_raw_mode(struct termios *original_termios) {
    struct termios new_termios;

    if (tcgetattr(STDIN_FILENO, original_termios) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }

    new_termios = *original_termios;

    new_termios.c_lflag &= ~(ICANON | ECHO);

    new_termios.c_cc[VMIN] = 1;

    new_termios.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

void reset_terminal_mode(struct termios *original_termios) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, original_termios) == -1) {
        perror("tcsetattr");
    }
}

int main() {
    struct termios original_termios;
    char answer;

    set_terminal_raw_mode(&original_termios);

    printf("Ваш любимый цвет? (Введите первую букву: К - Красный, С - Синий, З - Зеленый): ");
    fflush(stdout);

    answer = getchar();

    reset_terminal_mode(&original_termios);

    printf("\nВы выбрали: %c\n", answer);

    return 0;
}