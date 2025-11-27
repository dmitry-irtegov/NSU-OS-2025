#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <termios.h> 

int main() {
    struct termios original_settings, new_settings;
    char answer;

    if (tcgetattr(STDIN_FILENO, &original_settings) == -1) {
        perror("Ошибка получения атрибутов (tcgetattr)");
        exit(EXIT_FAILURE);
    }

    new_settings = original_settings;
    new_settings.c_lflag &= ~(ICANON);
    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_settings) == -1) {
        perror("Ошибка установки атрибутов (tcsetattr)");
        exit(EXIT_FAILURE);
    }

    printf("Просто вопрос? (y/n): ");
    fflush(stdout); 
    if (read(STDIN_FILENO, &answer, 1) == -1) {
        perror("Ошибка чтения");
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &original_settings) == -1) {
        perror("Ошибка восстановления атрибутов");
        exit(EXIT_FAILURE);
    }
    printf("\nВы выбрали: %c\n", answer);

    return 0;
}