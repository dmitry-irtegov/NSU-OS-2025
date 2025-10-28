#include <stdio.h>
#include <termios.h>
#include <unistd.h>

int set_term_attrs(int fd, struct termios *attr_old) {
    if (tcgetattr(fd, attr_old)) {
        return -1;
    }

    struct termios attr_new = *attr_old;
    attr_new.c_lflag &= ~ICANON & ~ECHO;
    attr_new.c_cc[VMIN] = 1;
    attr_new.c_cc[VTIME] = 0;

    if (tcsetattr(fd, TCSANOW, &attr_new)) {
        return -1;
    }
    return 0;
}

int restore_term_attrs(int fd, struct termios *attr_old) {
    return tcsetattr(fd, TCSANOW, attr_old);
}

int read_yes_no(int read_fd, int write_fd, char *result) {
    while (1) {
        int read_count = read(read_fd, result, 1);
        if (read_count == -1) {
            return -1;
        }
        if (read_count == 0) {
            *result = 'n';
            break;
        }
        if (*result == 'y' || *result == 'n') {
            break;
        }
        const char bell = '\07';
        if (write(write_fd, &bell, 1) != 1) {
            return -1;
        }
    }
    const char echo_msg[] = { *result, '\n' };
    if (write(write_fd, echo_msg, sizeof(echo_msg)) != sizeof(echo_msg)) {
        return -1;
    }
    return 0;
}

int main() {
    int in_fd = fileno(stdin);
    int out_fd = fileno(stdout);

    if (!isatty(in_fd)) {
        fprintf(stderr, "stdin is not a terminal.\n");
        return 1;
    }

    struct termios attrs_old;
    if (set_term_attrs(in_fd, &attrs_old)) {
        perror("Could not set terminal attributes");
        return 1;
    }

    static const char prompt_msg[] = "Yes or no? (y/n) ";
    if (write(out_fd, prompt_msg, sizeof(prompt_msg) - 1) != sizeof(prompt_msg) - 1) {
        perror("Could not print prompt message");
        if (restore_term_attrs(in_fd, &attrs_old)) {
            perror("Could not restore terminal attributes");
        }
        return 1;
    }

    char result;
    if (read_yes_no(in_fd, out_fd, &result)) {
        perror("Could not read response");
        if (restore_term_attrs(in_fd, &attrs_old)) {
            perror("Could not restore terminal attributes");
        }
        return 1;
    }

    if (restore_term_attrs(in_fd, &attrs_old)) {
        perror("Could not restore terminal attributes");
        return 1;
    }

    return 0;
}
