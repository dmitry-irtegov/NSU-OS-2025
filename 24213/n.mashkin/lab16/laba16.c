#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

int main() {
    struct termios old, new;

    if (tcgetattr(STDIN_FILENO, &old) == -1) {
        perror("Could not get attributes");
        exit(-1);
    }
    new = old;
    new.c_lflag &= ~ICANON;
    new.c_cc[VMIN] = 1;
    new.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new) == -1) {
        perror("Could not set attributes");
        exit(-1);
    }

    char answer;
    printf("English or Spanish? (e/s): ");
    
    answer = getchar();
    
    printf("\nYour choice was: %c\n", answer);
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &old) == -1) {
        perror("Could not set back initial attributes");
        exit(-1);
    }
    
    exit(0);
}
