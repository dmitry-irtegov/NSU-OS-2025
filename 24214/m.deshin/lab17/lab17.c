#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <stdint.h>

#define MAX_LEN 40

int set_term_attr(struct termios *old_term) {
    struct termios new_term;

    if (tcgetattr(STDIN_FILENO, old_term) == -1) {
        perror("tcgetattr failed");
        return -1;
    }

    new_term = *old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    new_term.c_cc[VMIN] = 1;
    new_term.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_term) == -1) {
        perror("tcsetattr failed");
        return -1;
    }

    return 0;
}

int restore_term_attr(struct termios *old_term) {
    if (tcsetattr(STDIN_FILENO, TCSANOW, old_term) == -1) {
        perror("tcsetattr failed");
        return 1;
    }

    return 0;
}

void handle_word_overflow(char *str, int *k, uint8_t c) {
    if (isspace(c)) {
        write(STDOUT_FILENO, "\007", 1);
        return;
    }

    int len = *k;
    int i = len - 1;

    while (i >= 0 && isspace((uint8_t)str[i])) {
        i--;
    }

    if (i < 0) {
        write(STDOUT_FILENO, "\007", 1);
        return;
    }

    int word_end = i;

    while (i >= 0 && !isspace((uint8_t)str[i])) {
        i--;
    }

    int word_start = i + 1;
    int word_len = word_end - word_start + 1;

    if (word_start == 0 || (word_len + 1 > MAX_LEN)) {
        write(STDOUT_FILENO, "\007", 1);
        return;
    }

    char word_buf[MAX_LEN + 1];
    for (int j = 0; j < word_len; j++) {
        word_buf[j] = str[word_start + j];
    }
    word_buf[word_len] = c;
    int new_word_len = word_len + 1;
    word_buf[new_word_len] = '\0';

    write(STDOUT_FILENO, &c, 1);

    int to_erase = new_word_len;
    for (int j = 0; j < to_erase; j++) {
        write(STDOUT_FILENO, "\b \b", 3);
    }

    write(STDOUT_FILENO, "\n", 1);

    for (int j = 0; j < new_word_len; j++) {
        write(STDOUT_FILENO, &word_buf[j], 1);
    }

    for (int j = 0; j < new_word_len; j++) {
        str[j] = word_buf[j];
    }
    *k = new_word_len;
}

void input_cycle() {
    char str[MAX_LEN + 1];
    int k = 0;
    uint8_t c;
    ssize_t n;

    while (1) {
        n = read(STDIN_FILENO, &c, 1);
        if (n <= 0) break;

        if (c == 4 && k == 0) break;

        if (isprint(c)) {
            if (k < MAX_LEN) {
                str[k++] = c;
                write(STDOUT_FILENO, &c, 1);
            } else {
                handle_word_overflow(str, &k, c);
            }
        } else {
            switch (c) {
                case 127:
                    if (k > 0) {
                        k--;
                        write(STDOUT_FILENO, "\b \b", 3);
                    } else {
                        write(STDOUT_FILENO, "\007", 1);
                    }
                    break;

                case 23:
                    if (k > 0) {
                        while (k > 0 && isspace((uint8_t)str[k - 1])) {
                            k--;
                            write(STDOUT_FILENO, "\b \b", 3);
                        }
                        while (k > 0 && !isspace((uint8_t)str[k - 1])) {
                            k--;
                            write(STDOUT_FILENO, "\b \b", 3);
                        }
                    } else {
                        write(STDOUT_FILENO, "\007", 1);
                    }
                    break;

                case 21:
                    while (k > 0) {
                        k--;
                        write(STDOUT_FILENO, "\b \b", 3);
                    }
                    break;

                case 4:
                    write(STDOUT_FILENO, "\007", 1);
                    break;
                
                default:
                    write(STDOUT_FILENO, "\007", 1);
            }
        }
    }

    str[k] = '\0';
    write(STDOUT_FILENO, "--- END ---\n", 12);
}

int main() {
    struct termios old_term;

    if (set_term_attr(&old_term) == -1) return 1;

    input_cycle();

    if (restore_term_attr(&old_term) == -1) return 1;

    return 0;
}
