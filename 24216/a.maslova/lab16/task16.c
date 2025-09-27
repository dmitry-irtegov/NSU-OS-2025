#include <stdio.h>
#include <termios.h>
#include <unistd.h>

char get_without_enter(){
    struct termios old, new;
    char ch;

    tcgetattr(STDIN_FILENO, &old);

    new = old;
    new.c_lflag &= ~ICANON;
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &new);

    read(STDIN_FILENO, &ch, 1);

    tcsetattr(STDIN_FILENO, TCSANOW, &old);
    return ch;
}

int main(){
    printf("Хотите продолжить? (y/n) ");
    fflush(stdout);

    char answer = get_without_enter();
    if (answer == 'y' || answer == 'Y'){
        printf("\nПродолжаем...");
    } else {
        printf("\nЗавершаем.");
    }
    return 0;
}