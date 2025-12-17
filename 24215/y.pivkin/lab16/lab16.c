#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>

int main () {
    struct termios oldt, newt;
    if(tcgetattr(fileno(stdin), &oldt) == -1){
        perror("tcgetattr");
        exit(1);
    }

    newt = oldt;

    newt.c_lflag &= ~(ICANON | ECHO);
    newt.c_cc[VMIN] = 1; // на всякий случай
    //printf("%d\n", newt.c_cc[VMIN]);

    if(tcsetattr(fileno(stdin), TCSANOW, &newt) == -1){
        perror("tcsetattr");
        exit(2);
    }

    //============================================
    printf("Heads or tails? ");
    char c = getchar();

    switch(c) {
        case 'h':
        case 'H':
            printf("You chose heads.");
            break;
        case 't':
        case 'T':
            printf("You chose tails.");
            break;
        default:
            printf("I don't know this symbol.");
            break;
    }
    //============================================

    if(tcsetattr(fileno(stdin), TCSANOW, &oldt) == -1){
        perror("tcsetattr");
        exit(3);
    }
    exit(0);
}
