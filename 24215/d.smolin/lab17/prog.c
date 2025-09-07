#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <string.h>

#define LINE_MAX      40
#define KEY_ERASE     0x7f
#define KEY_BACKSPACE 0x08
#define KEY_KILL      0x15
#define KEY_CTRL_W    0x17
#define KEY_CTRL_D    0x04
#define KEY_BELL      0x07

int main() {
    struct termios orig, raw;
    tcgetattr(STDIN_FILENO, &orig);
    raw = orig;
    raw.c_lflag &= ~(ICANON | ECHO | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    char line[LINE_MAX + 1];
    int len = 0;

    while (1) {
        char c;
        int rd = read(STDIN_FILENO, &c, 1);
        if (rd <= 0) break;

        if (c == KEY_ERASE || c == KEY_BACKSPACE) {
            if (len > 0) {
                len--;
                write(STDOUT_FILENO, "\b \b", 3);
            } else {
                write(STDOUT_FILENO, "\a", 1);
            }
        }
        else if (c == KEY_KILL) {
            while (len > 0) {
                write(STDOUT_FILENO, "\b \b", 3);
                len--;
            }
        }
        else if (c == KEY_CTRL_W) {
            while (len > 0 && line[len-1] == ' ') {
                write(STDOUT_FILENO, "\b \b", 3);
                len--;
            }
            while (len > 0 && line[len-1] != ' ') {
                write(STDOUT_FILENO, "\b \b", 3);
                len--;
            }
        }
        else if (c == '\n' || c == '\r') {
            write(STDOUT_FILENO, "\n", 1);
            len = 0;
        }
        else if (c == KEY_CTRL_D) {
            if (len == 0) break;
        }
        else if (c == KEY_BELL) {
            write(STDOUT_FILENO, "\a", 1);
        }
        else if (isgraph(c) || c == ' ') {
            line[len++] = c;
            write(STDOUT_FILENO, &c, 1);
            if (len > LINE_MAX) {
                int j = LINE_MAX - 1;
                while (j >= 0 && line[j] != ' ') j--;
                int word_start = (j < 0) ? LINE_MAX : j+1;
                write(STDOUT_FILENO, "\n", 1);
                for (int k = word_start; k < len; k++)
                    write(STDOUT_FILENO, &line[k], 1);
                int word_len = len - word_start;
                memmove(line, line + word_start, word_len);
                len = word_len;
            }
        }

        else {
            write(STDOUT_FILENO, "\a", 1);
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &orig);
    return 0;
}
