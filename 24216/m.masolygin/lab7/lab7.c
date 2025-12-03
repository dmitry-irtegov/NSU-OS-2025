#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_LINES 10000

typedef struct line_info {
    off_t offset;
    size_t length;
} line_info_t;

line_info_t line_table[MAX_LINES];
int total_lines = 0;

char* file_data = NULL;
size_t file_size = 0;

int build_line_table(const char* filename) {
    int fd;
    struct stat sb;
    off_t file_offset = 0;
    off_t line_start = 0;

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        return -1;
    }

    file_size = sb.st_size;

    if (file_size == 0) {
        close(fd);
        return 0;
    }

    file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }

    close(fd);

    for (size_t i = 0; i < file_size; i++) {
        if (file_offset == line_start) {
            if (total_lines < MAX_LINES) {
                line_table[total_lines].offset = file_offset;
                total_lines++;
            } else {
                fprintf(stderr, "Warning: file has more than %d lines\n",
                        MAX_LINES);
                return 0;
            }
        }

        file_offset++;

        if (file_data[i] == '\n') {
            line_table[total_lines - 1].length = file_offset - line_start - 1;
            line_start = file_offset;
        }
    }

    if (file_offset > line_start && total_lines > 0) {
        line_table[total_lines - 1].length = file_offset - line_start;
    }

    return 0;
}

int print_line(int line_number) {
    if (line_number < 1 || line_number > total_lines) {
        fprintf(stderr, "Error: line number out of range (1-%d)\n",
                total_lines);
        return -1;
    }

    int index = line_number - 1;

    char* line_start = file_data + line_table[index].offset;
    size_t length = line_table[index].length;

    printf("Line %d: %.*s\n", line_number, (int)length, line_start);

    return 0;
}

void print_all_lines() {
    printf("\n=== Timeout! Printing file:\n");

    for (int i = 1; i <= total_lines; i++) {
        print_line(i);
    }

    printf("=== End of file\n");
}

void alarm_handler(int signo) {
    if (file_data != NULL) {
        print_all_lines();
        munmap(file_data, file_size);
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

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    filename = argv[1];

    if (build_line_table(filename) == -1) {
        return 1;
    }

    printf("File has %d lines.\n", total_lines);

    if (total_lines == 0) {
        printf("File is empty.\n");
        return 0;
    }

    if (signal(SIGALRM, alarm_handler) == SIG_ERR) {
        perror("signal");
        munmap(file_data, file_size);
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

        print_line(line_number);

        invite_user();
        alarm(5);
    }

    alarm(0);

    if (file_data != NULL) {
        munmap(file_data, file_size);
    }

    return 0;
}
