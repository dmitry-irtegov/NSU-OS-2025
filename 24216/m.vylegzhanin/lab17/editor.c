#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <string.h>

#define MAX_LINE 40
#define CTRL_D 4
#define CTRL_G 7
#define CTRL_W 23
#define KEY_ERASE 256
#define KEY_KILL 257

static int classify_input(unsigned char c, char erase_char, char kill_char) {
    if (c == erase_char) {
        return KEY_ERASE;
    }
    if (c == kill_char) {
        return KEY_KILL;
    }
    return c;
}

int main() {
    struct termios old_term, new_term;
    char line[MAX_LINE + 1];
    int pos = 0;
    int col = 0;
    unsigned char c;
    char erase_char, kill_char;

    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    
    erase_char = old_term.c_cc[VERASE];
    kill_char = old_term.c_cc[VKILL];
    
    new_term.c_lflag &= ~(ICANON | ECHO);
    new_term.c_cc[VMIN] = 1;
    new_term.c_cc[VTIME] = 0;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    while (read(STDIN_FILENO, &c, 1) == 1) {
        int action = classify_input(c, erase_char, kill_char);

        switch (action) {
            case CTRL_D:
                if (pos == 0) {
                    goto exit_loop;
                }
                break;

            case KEY_ERASE:
                if (pos > 0) {
                    pos--;
                    col--;
                    write(STDOUT_FILENO, "\b \b", 3);
                }
                continue;

            case KEY_KILL:
                while (pos > 0) {
                    pos--;
                    col--;
                    write(STDOUT_FILENO, "\b \b", 3);
                }
                continue;

            case CTRL_W:
                while (pos > 0 && line[pos - 1] == ' ') {
                    pos--;
                    col--;
                    write(STDOUT_FILENO, "\b \b", 3);
                }
                while (pos > 0 && line[pos - 1] != ' ') {
                    pos--;
                    col--;
                    write(STDOUT_FILENO, "\b \b", 3);
                }
                continue;

            case '\n':
                write(STDOUT_FILENO, "\n", 1);
                pos = 0;
                col = 0;
                continue;

            default:
                if (!isprint(c)) {
                    char bell = CTRL_G;
                    write(STDOUT_FILENO, &bell, 1);
                    continue;
                }
                break;
        }

        if (col >= MAX_LINE) {
            int word_start = pos;
            while (word_start > 0 && line[word_start - 1] != ' ') {
                word_start--;
            }

            int should_wrap = (word_start > 0 && pos > word_start);
            if (!should_wrap) {
                write(STDOUT_FILENO, "\n", 1);
                pos = 0;
                col = 0;
            } else {
                int chars_to_move = pos - word_start;
                char word[MAX_LINE + 1];
                memcpy(word, &line[word_start], chars_to_move);

                for (int i = 0; i < chars_to_move; i++) {
                    write(STDOUT_FILENO, "\b \b", 3);
                }

                write(STDOUT_FILENO, "\n", 1);

                memcpy(line, word, chars_to_move);
                pos = chars_to_move;
                col = chars_to_move;
                write(STDOUT_FILENO, word, chars_to_move);
            }
        }

        line[pos++] = c;
        col++;
        write(STDOUT_FILENO, &c, 1);
    }

exit_loop:

    write(STDOUT_FILENO, "\n", 1);
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    
    return 0;
}
