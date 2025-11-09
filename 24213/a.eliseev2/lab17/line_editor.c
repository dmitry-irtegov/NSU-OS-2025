#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define LENGTH_MAX 40
#define RESULT_OK 0
#define RESULT_ERROR -1
#define RESULT_END -2

typedef struct {
    char chars[LENGTH_MAX];
    int length;
    int out_fd;
} editor_t;

void editor_init(editor_t *editor, int out_fd) {
    editor->out_fd = out_fd;
    editor->length = 0;
}

int last_word_length(editor_t *editor) {
    int start_index = editor->length - 1;
    for (; start_index >= 0; start_index--) {
        if (editor->chars[start_index] != ' ') {
            break;
        }
    }
    for (; start_index >= 0; start_index--) {
        if (editor->chars[start_index] == ' ') {
            break;
        }
    }
    start_index++;
    return editor->length - start_index;
}

int append_chars(editor_t *editor, char *chars, int count) {
    memcpy(editor->chars + editor->length, chars, count);
    editor->length += count;
    if (write(editor->out_fd, chars, count) < 0) {
        return RESULT_ERROR;
    } else {
        return RESULT_OK;
    }
}

int erase_chars(editor_t *editor, int count) {
    if (count > editor->length) {
        count = editor->length;
    }
    const char bs_chars[] = {'\x08', ' ', '\x08'};
    for (int i = 0; i < count; i++) {
        if (write(editor->out_fd, bs_chars, 3) < 0) {
            return RESULT_ERROR;
        }
    }
    editor->length -= count;
    return RESULT_OK;
}

int erase_line(editor_t *editor) {
    return erase_chars(editor, editor->length);
}

int erase_word(editor_t *editor) {
    int length = last_word_length(editor);
    return erase_chars(editor, length);
}

int begin_new_line(editor_t *editor) {
    const char newline = '\n';
    if (write(editor->out_fd, &newline, 1) < 0) {
        return RESULT_ERROR;
    }
    editor->length = 0;
    return RESULT_OK;
}

int wrap_last_word(editor_t *editor) {
    int length = last_word_length(editor);
    if (editor->length == 0 || length == editor->length ||
        editor->chars[editor->length - 1] == ' ') {
        return begin_new_line(editor);
    }
    char word[LENGTH_MAX];
    int start_index = editor->length - length;
    memcpy(word, editor->chars + start_index, length);
    if (erase_chars(editor, length)) {
        return RESULT_ERROR;
    }
    if (begin_new_line(editor)) {
        return RESULT_ERROR;
    }
    return append_chars(editor, word, length);
}

int process_char(editor_t *editor, struct termios *attr, char ch) {
    if (ch == attr->c_cc[VERASE]) {
        return erase_chars(editor, 1);
    } else if (ch == attr->c_cc[VKILL]) {
        return erase_line(editor);
    } else if (ch == '\x17') { // ^W
        return erase_word(editor);
    } else if (ch == '\x04') { // ^D
        return editor->length == 0 ? RESULT_END : RESULT_OK;
    } else if (ch == ' ') {
        if (editor->length == LENGTH_MAX) {
            return begin_new_line(editor);
        } else {
            return append_chars(editor, &ch, 1);
        }
    } else if (isgraph(ch)) {
        if (editor->length == LENGTH_MAX) {
            if (wrap_last_word(editor)) {
                return RESULT_ERROR;
            }
        }
        return append_chars(editor, &ch, 1);
    } else {
        const char bell = '\a';
        if (write(editor->out_fd, &bell, 1) < 0) {
            return RESULT_ERROR;
        }
        return RESULT_OK;
    }
}

int set_term_attrs(int fd, struct termios *attr_new, struct termios *attr_old) {
    if (tcgetattr(fd, attr_old)) {
        return -1;
    }

    *attr_new = *attr_old;
    attr_new->c_lflag &= ~ICANON & ~ECHO & ~ISIG;
    attr_new->c_iflag &= ~IXON;
    attr_new->c_cc[VMIN] = 1;
    attr_new->c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, attr_new)) {
        return -1;
    }
    return 0;
}

int restore_term_attrs(int fd, struct termios *attr_old) {
    return tcsetattr(fd, TCSANOW, attr_old);
}

int main() {
    int in_fd = fileno(stdin);
    int out_fd = fileno(stdout);

    if (!isatty(in_fd)) {
        fprintf(stderr, "stdin is not a terminal.\n");
        return 1;
    }

    struct termios attrs_new;
    struct termios attrs_old;
    if (set_term_attrs(in_fd, &attrs_new, &attrs_old)) {
        perror("Could not set terminal attributes");
        return 1;
    }

    bool is_error;
    editor_t editor;
    editor_init(&editor, out_fd);

    while (true) {
        char ch;

        int read_result = read(in_fd, &ch, 1);
        is_error = read_result < 0;
        if (read_result <= 0) {
            break;
        }

        int process_result = process_char(&editor, &attrs_new, ch);
        is_error = process_result == RESULT_ERROR;
        if (process_result != RESULT_OK) {
            break;
        }
    }

    if (is_error) {
        perror("Error while processing character");
    }

    if (restore_term_attrs(in_fd, &attrs_old)) {
        perror("Could not restore terminal attributes");
        is_error = true;
    }

    return is_error ? 1 : 0;
}
