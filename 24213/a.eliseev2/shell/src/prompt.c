#include "io.h"
#include <netdb.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/param.h>
#include <unistd.h>

static void print_prompt() {
    char *login = getlogin();
    char hostname[MAXHOSTNAMELEN + 1];
    int hostname_res = gethostname(hostname, sizeof(hostname));
    hostname[MAXHOSTNAMELEN] = 0;
    if (login && !hostname_res) {
        fdprintf(1, "[%s@%s]> ", login, hostname);
    } else {
        fdprintf(1, "> ");
    }
}

static int discard_rest(char *buffer, int buffer_size, char carry_bslash,
                        char *is_eof) {
    while (1) {
        int read_count = read(0, buffer, buffer_size - 1);
        switch (read_count) {
        case -1:
            perror("A read error ocurred");
            return 1;
        case 0:
            *is_eof = 1;
            return 0;
        case 1:
            if (carry_bslash) {
                carry_bslash = 0;
                continue;
            }
            if (buffer[0] == '\n') {
                return 0;
            }
        default:
            carry_bslash = 0;
            if (buffer[read_count - 1] == '\n' &&
                buffer[read_count - 2] != '\\') {
                return 0;
            }
            if (buffer[read_count - 1] == '\\') {
                carry_bslash = 1;
            }
            break;
        }
    }
}

static int read_line(char *buffer, int buffer_size, char *is_eof) {
    int line_length = 0;
    while (1) {
        int read_count =
            read(0, buffer + line_length, buffer_size - line_length - 1);
        if (read_count == -1) {
            perror("A read error ocurred");
            return -1;
        }
        line_length += read_count;
        buffer[line_length] = '\0';

        if (!read_count) {
            // EOF
            *is_eof = 1;
            break;
        }

        if (line_length >= 2 && buffer[line_length - 2] == '\\' &&
            buffer[line_length - 1] == '\n') {
            // Line should be continued
            buffer[line_length - 2] = ' ';
            buffer[line_length - 1] = '\0';
            line_length -= 1;
            continue;
        }

        if (line_length >= 1 && buffer[line_length - 1] == '\n') {
            // Success
            break;
        }

        if (line_length == buffer_size - 1) {
            // Buffer overflow
            break;
        }
    }
    return line_length;
}

int prompt_line(char *buffer, int buffer_size, char *is_eof) {
    if (buffer_size == 0) {
        return 0;
    }

    while (1) {
        if (*is_eof) {
            return 0;
        }

        print_prompt();

        int length = read_line(buffer, buffer_size, is_eof);
        if (length <= 0) {
            // Error or immediate EOF
            return 0;
        }
        if (buffer[length - 1] == '\n' || *is_eof) {
            // Success
            return 1;
        }

        // The line is too long to fit into the buffer, discard the rest.
        char carry_bslash = buffer[length - 1] == '\\';
        int discard_result =
            discard_rest(buffer, buffer_size, carry_bslash, is_eof);
        fprintf(stderr, "Line too long!\n");

        if (discard_result) {
            return 0;
        }
    }
}
