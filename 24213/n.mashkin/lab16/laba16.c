#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

int main() {
    if (isatty(STDIN_FILENO) == 0) {
        fprintf(stderr, "stdin is not a terminal");
        exit(-1);
    }

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

    printf("English or Spanish? (e/s): ");
    
    int answer = getchar();
    
    if (answer != EOF) {
        printf("\nYour choice was: %c\n", answer);
    } else {
        fprintf(stderr, "An error while processing answer has occured or an EOF is reached");
    }
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &old) == -1) {
        perror("Could not set back initial attributes");
        exit(-1);
    }
    
    exit(0);
}
