#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>


int main() {
    struct termios old, new;

    if (tcgetattr(fileno(stdin), &old) == -1) {
        perror("Error getting terminal attributes");
        exit(EXIT_FAILURE);
    }
    new = old;

    new.c_lflag &= ~(ICANON);
    new.c_cc[VMIN] = 1;
    
    if (tcsetattr(fileno(stdin), TCSANOW, &new) == -1) {
        perror("Error setting terminal attributes");
        exit(EXIT_FAILURE);
    }

    char symbol;
    printf("Press any one symbol: ");
    symbol = fgetc(stdin);
    if (symbol == EOF) {
        perror("Error reading character");
        exit(EXIT_FAILURE);
    }
    printf("\nYou pressed: %c\n", symbol);

    if (tcsetattr(fileno(stdin), TCSANOW, &old) == -1) {
        perror("Error restoring terminal attributes");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}