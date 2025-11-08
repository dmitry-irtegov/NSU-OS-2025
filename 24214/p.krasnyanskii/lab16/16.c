#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

char reply(){
    char c;
    struct termios tty, savetty;
     if (fflush(stdout) == EOF) {
        perror("fflush");
        exit(EXIT_FAILURE);
    }

    if (tcgetattr(STDIN_FILENO, &tty) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }

    savetty = tty;
    tty.c_lflag &= ~(ISIG | ICANON);
    tty.c_cc[VMIN] = 1;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }

    ssize_t r = read(STDIN_FILENO, &c, 1);
    if (r < 0) {
        perror("read");
        tcsetattr(STDIN_FILENO, TCSANOW, &savetty);
        exit(EXIT_FAILURE);
    }

    if (tcsetattr(STDIN_FILENO, TCSANOW, &savetty) == -1) {
        perror("tcsetattr (restore settings)");
        exit(EXIT_FAILURE);
    }

    return c;
}

int main(void){
    if (printf("Select any symbol: ") < 0) {
        perror("printf");
        exit(EXIT_FAILURE);
    }

    char c = reply();

    if (printf("\nYou've selected: %c\n", c) < 0) {
        perror("printf");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

