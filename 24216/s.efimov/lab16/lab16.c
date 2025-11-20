#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

int ask_question(char* answer) {
    struct termios old, new;

    if (tcgetattr(STDIN_FILENO, &old) == -1) {
        return -1;
    }
    
    new = old;
    new.c_lflag &= ~ICANON; 
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new)) {
        return -1;
    }
    
    int char_read = read(STDIN_FILENO, answer, 1);
    int status = tcsetattr(STDIN_FILENO, TCSANOW, &old);

    if (char_read == (-1) || status != 0) {
        return -1;
    }
    
    return 0;
}

int main() {
    char answer;

    printf("Продолжаем? (y/n): ");
    fflush(stdout);

    if (ask_question(&answer) == (-1)) {
        perror("Error input");
        exit(EXIT_FAILURE);
    }

    if (answer == 'y' || answer == 'Y') {
        printf("\nВы выбрали продолжить.\n");
    } else if (answer == 'n' || answer == 'N') {
        printf("\nВы выбрали не продолжать.\n");
    } else {
        printf("\nНеверный ввод.\n");
    }
    exit(EXIT_SUCCESS);
}
