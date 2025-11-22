#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_LINE_LENGTH 40

struct termios original_termios;

void set_terminal(int old) {
    if (old) {
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios)) {
            perror("tcsetattr");
            exit(-1);
        }
        return;
    }

    struct termios new_termios;
    
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        perror("tcgetattr");
        exit(-1);
    }
    
    new_termios = original_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
}

int is_printable(char c) {
    return (c >= 32 && c <= 126);
}

int is_whitespace(char c) {
    return (c == ' ' || c == '\t');
}

void remove_last_word(char *line, int *pos) {
    int i = *pos - 1;
    
    while (i >= 0 && is_whitespace(line[i])) {
        i--;
    }
    
    while (i >= 0 && !is_whitespace(line[i])) {
        i--;
    }
    
    *pos = i + 1;
}

void carry_word(char *line, int *pos) {
    int word_start = 0;
    for (int i = *pos; i >= 0; i--) {
        if (is_whitespace(line[i])) {
            word_start = i + 1;
            break;
        }
    }
    if (!word_start) {
        return;
    }

    *pos -= word_start;
    for (int i = 0; i < *pos; i++) {
        line[i] = line[word_start + i];
        write(STDOUT_FILENO, "\b \b", 3);
    }
    line[*pos] = '\0';

    write(STDOUT_FILENO, "\n", 1);
    
    write(STDOUT_FILENO, line, *pos);
}

int main() {
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fprintf(stderr, "Please launch from terminal");
        exit(-1);
    }

    char line[MAX_LINE_LENGTH + 1] = {0};
    int pos = 0;
    int return_val = 0;

    set_terminal(0);
    
    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) == -1) {
            perror("read");
            return_val = -1;
            break;
        }
        tcflush(STDIN_FILENO, TCIFLUSH);
        
        // CTRL+D
        if (c == 4) {
            if (pos == 0) {
                break;
            }
            if (write(STDOUT_FILENO, "\a", 1) == -1) {
                perror("write");
                return_val = -1;
                break;
            }
            continue;
        }
        
        // ERASE
        if (c == 127 || c == 8) {
            if (pos > 0) {
                pos--;
                if (write(STDOUT_FILENO, "\b \b", 3) == -1) {
                    perror("write");
                    return_val = -1;
                    break;
                }
            }
            continue;
        }
        
        // KILL
        if (c == 21) {
            if (pos > 0) {
                for (int i = 0; i < pos; i++) {
                    if (write(STDOUT_FILENO, "\b \b", 3) == -1) {
                        perror("write");
                        return_val = -1;
                        break;
                    }
                }
                pos = 0;
            }
            continue;
        }
        
        // CTRL+W
        if (c == 23) {
            if (pos > 0) {
                int old_pos = pos;
                remove_last_word(line, &pos);
                
                for (int i = old_pos; i > pos; i--) {
                    if (write(STDOUT_FILENO, "\b \b", 3) == -1) {
                        perror("write");
                        return_val = -1;
                        break;
                    }
                }
            }
            continue;
        }
        
        if (!is_printable(c)) {
            if (write(STDOUT_FILENO, "\a", 1) == -1) {
                perror("write");
                return_val = -1;
                break;
            }
            continue;
        }
        
        if (pos == MAX_LINE_LENGTH) {
            if (is_whitespace(c)) {
                continue;
            }
            carry_word(line, &pos);
        }
        
        if (pos < MAX_LINE_LENGTH) {
            line[pos] = c;
            pos++;
            if (write(STDOUT_FILENO, &c, 1) == -1) {
                perror("write");
                return_val = -1;
                break;
            }
        }
    }

    set_terminal(1);
    
    return return_val;
}
