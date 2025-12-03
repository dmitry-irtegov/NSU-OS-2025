#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_LINES 10000
#define BUF_SIZE 4096

typedef struct line_info {
    off_t offset;
    size_t length;
} line_info_t;

line_info_t line_table[MAX_LINES];
int total_lines = 0;

int global_fd = -1;

int build_line_table(const char* filename) {
    int fd;
    char buffer[BUF_SIZE];
    ssize_t bytes_read;
    off_t file_offset = 0;
    off_t line_start = 0;

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    while ((bytes_read = read(fd, buffer, BUF_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (file_offset == line_start) {
                if (total_lines < MAX_LINES) {
                    line_table[total_lines].offset = file_offset;
                    total_lines++;
                } else {
                    fprintf(stderr, "Warning: file has more than %d lines\n",
                            MAX_LINES);
                    close(fd);
                    return 0;
                }
            }

            file_offset++;

            if (buffer[i] == '\n') {
                line_table[total_lines - 1].length =
                    file_offset - line_start - 1;
                line_start = file_offset;
            }
        }
    }

    if (bytes_read == -1) {
        perror("read");
        close(fd);
        return -1;
    }

    if (file_offset > line_start && total_lines > 0) {
        line_table[total_lines - 1].length = file_offset - line_start;
    }

    close(fd);
    return 0;
}

int print_line(int fd, int line_number) {
    char* buffer;
    ssize_t bytes_read;

    if (line_number < 1 || line_number > total_lines) {
        fprintf(stderr, "Error: line number out of range (1-%d)\n",
                total_lines);
        return -1;
    }

    int index = line_number - 1;

    if (lseek(fd, line_table[index].offset, SEEK_SET) == -1) {
        perror("lseek");
        return -1;
    }

    buffer = (char*)malloc(line_table[index].length + 1);
    if (buffer == NULL) {
        perror("malloc");
        return -1;
    }

    bytes_read = read(fd, buffer, line_table[index].length);
    if (bytes_read == -1) {
        perror("read");
        free(buffer);
        return -1;
    }

    buffer[bytes_read] = '\0';

    printf("Line %d: %s\n", line_number, buffer);

    free(buffer);
    return 0;
}

void print_all_lines(int fd) {
    printf("\n=== Timeout! Printing file:\n");

    for (int i = 1; i <= total_lines; i++) {
        print_line(fd, i);
    }

    printf("=== End of file\n");
}

void alarm_handler(int signo) {
    if (global_fd != -1) {
        print_all_lines(global_fd);
        close(global_fd);
    }

    exit(0);
}

void invite_user() {
    printf("\nYou have 5 seconds to enter line number (0 to quit): ");
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    char* filename;
    int line_number;
    int fd;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    filename = argv[1];

    if (build_line_table(filename) == -1) {
        return 1;
    }

    printf("File has %d lines.\n", total_lines);

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    global_fd = fd;

    if (signal(SIGALRM, alarm_handler) == SIG_ERR) {
        perror("signal");
        close(fd);
        return 1;
    }

    invite_user();

    alarm(5);

    while (scanf("%d", &line_number) == 1) {
        alarm(0);

        if (line_number == 0) {
            printf("Exit\n");
            break;
        }

        print_line(fd, line_number);

        invite_user();
        alarm(5);
    }

    alarm(0);
    close(fd);

    return 0;
}
