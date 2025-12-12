#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

int main() {
    struct termios oldt, newt;
    char answer;

    if (tcgetattr(STDIN_FILENO, &oldt) != 0) {
        perror("Error tcgetattr");
        return 1;
    }

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); 
    newt.c_cc[VMIN] = 1; 
    newt.c_cc[VTIME] = 0; 

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &newt) != 0) {
        perror("Error tcsetattr");
        return 1;
    }

    printf("Do you like NSU? (y/n): ");
    fflush(stdout);

    ssize_t n = read(STDIN_FILENO, &answer, 1);
    if (n == -1) {
        perror("Read error");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return 1;
    } else if (answer != 'y' && answer != 'n') {
        fprintf(stderr, "Invalid input: expected 'y' or 'n'\n");
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return 1;
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) != 0) {
        perror("Terminal recovery error");
        return 1;
    }

    printf("\nAnswer: %c\n", answer);
    return 0;
}

