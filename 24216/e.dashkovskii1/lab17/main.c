#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEN 40

void set_terminal_mode(struct termios *old_termios) {
    struct termios new_termios;

    if (tcgetattr(STDIN_FILENO, old_termios) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }

    new_termios = *old_termios;

    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

void restore_terminal_mode(struct termios *old_termios) {
    if (tcsetattr(STDIN_FILENO, TCSANOW, old_termios) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

void bell() {
    write(STDOUT_FILENO, "\007", 1); // CTRL-G
}

void process_input() {
    char line[MAX_LEN + 1];  
    int len = 0;
    char c;

    while (1) {
        c = getchar();

        switch (c) {
            case 4: // CTRL-D
                if (len == 0) {
                    return;  
                } else {
                    bell();
                }
                break;

            case 127: // ERASE (backspace)
                if (len > 0) {
                    len--;
                    printf("\b \b");
                }
                break;

            case 23: // CTRL-W
                if (len > 0) {
                    while (len > 0 && isspace(line[len - 1])) {
                        len--;
                        printf("\b \b");
                    }
                    while (len > 0 && !isspace(line[len - 1])) {
                        len--;
                        printf("\b \b");
                    }
                }
                break;

            case 21: // CTRL-U (kill)
                while (len > 0) {
                    len--;
                    printf("\b \b");
                }
                break;

            default:
                if (isprint(c)) {
                    if (len < MAX_LEN) {
                        line[len++] = c;
                        putchar(c);
                    } else {
                        bell();
                    }
                } else {
                    bell();
                }
                break;
        }
    }

    line[len] = '\0';
    printf("End\n");
}

int main() {
    struct termios old_termios;

    set_terminal_mode(&old_termios);

    process_input();

    restore_terminal_mode(&old_termios);

    return 0;
}
