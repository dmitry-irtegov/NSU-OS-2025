#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>

static struct termios old_terminal;

void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal);
}

void handle_signal(int sig) {
    restore_terminal();
    exit(EXIT_FAILURE);
}

int main(void) {
    struct termios new_terminal;
    char input;
    ssize_t r;

    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "Error: stdin is not a terminal\n");
        return EXIT_FAILURE;
    }
    if (tcgetattr(STDIN_FILENO, &old_terminal) == -1) {
        perror("tcgetattr failed");
        return EXIT_FAILURE;
    }
    if (atexit(restore_terminal) != 0) {
        fprintf(stderr, "atexit registration failed\n");
        return EXIT_FAILURE;
    }

    signal(SIGINT, handle_signal);  
    signal(SIGTERM, handle_signal);
    if (signal(SIGINT, handle_signal) == SIG_ERR) {
    perror("Failed to set SIGINT handler");
    return EXIT_FAILURE;
}
    
    new_terminal = old_terminal;
    new_terminal.c_lflag &= ~(ICANON | ECHO); 
    new_terminal.c_cc[VMIN] = 1;
    new_terminal.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_terminal) == -1) {
        perror("tcsetattr failed");
        return EXIT_FAILURE;
    }

    printf("Do you want to continue? (y/n): ");
    fflush(stdout);

    r = read(STDIN_FILENO, &input, 1);
    if (r == -1) {
        perror("read failed");
        return EXIT_FAILURE;
    }
    if (r == 0) {
        fprintf(stderr, "EOF received instead of input\n");
        return EXIT_FAILURE;
    }
    printf("\nYou pressed: %c\n", input);
    return EXIT_SUCCESS;
}
