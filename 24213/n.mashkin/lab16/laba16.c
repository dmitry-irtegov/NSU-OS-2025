#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

int main() {
    struct termios old, new;

    if (tcgetattr(STDIN_FILENO, &old) == -1) {
        printf("Could not get attributes\n");
        exit(1);
    }
    new = old;
    new.c_lflag &= ~ICANON;
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new) == -1) {
        printf("Could not set attributes\n");
        exit(1);
    }

    char answer;
    printf("English or Spanish? (e/s): ");
    
    answer = getchar();
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &old) == -1) {
        printf("Could not set back initial attributes\n");
        exit(1);
    }
    
    printf("\nYour choice was: %c\n", answer);
    
    exit(0);
}
