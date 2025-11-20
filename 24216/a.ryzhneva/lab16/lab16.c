#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>

int main() {
    struct termios original_settings_term, changed_settings_term;

    printf("Yes (y) or No (n): ");
    fflush(stdout);

    if (!isatty(STDIN_FILENO)) { 
        fprintf(stderr, "terminal not interractive"); 
        exit(EXIT_FAILURE); 
    }

    if (tcgetattr(STDIN_FILENO, &original_settings_term) != 0) { 
        perror("tcgetattr failed"); 
        exit(EXIT_FAILURE); 
    }

    changed_settings_term = original_settings_term;

    changed_settings_term.c_lflag &= ~ICANON;
    changed_settings_term.c_cc[VMIN] = 1;
    changed_settings_term.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &changed_settings_term) != 0) {
        perror("tcsetattr failed");
        exit(EXIT_FAILURE);
    }

    char in;
    if (read(STDIN_FILENO, &in, 1) == -1) {
        perror("read failed");
        tcsetattr(STDIN_FILENO, TCSANOW, &original_settings_term);
        exit(EXIT_FAILURE);
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &original_settings_term) != 0) {
        perror("tcsetattr failed");
        exit(EXIT_FAILURE);
    }

    printf("\n");
    return 0;
}