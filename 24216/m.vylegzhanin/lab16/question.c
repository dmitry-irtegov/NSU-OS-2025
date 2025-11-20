#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

int main() {
    struct termios old_term, new_term;
    char answer;
    
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    
    new_term.c_lflag &= ~(ICANON | ECHO);
    new_term.c_cc[VMIN] = 1;
    new_term.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);
    
    printf("Do you want to continue? (y/n): ");
    fflush(stdout);
    
    read(STDIN_FILENO, &answer, 1);
    
    printf("\nYou answered: %c\n", answer);
    
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    
    if (answer == 'y' || answer == 'Y') {
        printf("Continuing...\n");
        return 0;
    } else {
        printf("Exiting...\n");
        return 1;
    }
}
