#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

int main(){
    struct termios oldt, newt;
    char c;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    printf("Are you dumb? (y/n) ");
    fflush(stdout);

    read(STDIN_FILENO, &c, 1);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    printf("\nYour input: %c\n", c);
}
