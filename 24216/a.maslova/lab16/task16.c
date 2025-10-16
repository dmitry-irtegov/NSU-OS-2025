#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

int get_without_enter(char* result){
    struct termios old, new;

    if (tcgetattr(STDIN_FILENO, &old) == -1) {
        return -1;
    }

    new = old;
    new.c_lflag &= ~ICANON;
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new) == -1) {
        return -1;
    }

    int read_result = read(STDIN_FILENO, result, 1);

    if (tcsetattr(STDIN_FILENO, TCSANOW, &old) == -1) {
        return -1;
    }

    return (read_result == 1) ? 0 : -1;
}

int main(){
    char answer;

    printf("Хотите продолжить? (y/n) ");
    fflush(stdout);

    if (get_without_enter(&answer) == -1) {
        perror("Input failed");
        exit(EXIT_FAILURE);
    }
    
    if (answer == 'y' || answer == 'Y'){
        printf("\nПродолжаем...\n");
    } else {
        printf("\nЗавершаем.\n");
    }

    return 0;
}
