#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

typedef struct {
    off_t offset;
    size_t length;
} lineInfo;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Can't open that file");
        exit(1);
    }

    size_t capacity = 16;
    lineInfo *table = malloc(capacity * sizeof(lineInfo));
    if (!table) {
        perror("Can't allocate memory for lines table");
        close(fd);
        exit(1);
    }
    table[0].offset = 0;
    table[0].length = 0;

    size_t line_count = 0, len = 0;
    off_t offset = 0;
    char ch;
    int n, eof_reached = 0;

    while (1) {
        printf("Enter line number (0 — exit): ");
        if (scanf("%d", &n) != 1) {
            printf("Input is not a valid number.\n");
            free(table);
            close(fd);
            exit(1);
        }

        if (n == 0) break;
        
        if (n < 0) {
            printf("Input must be positive\n");
            continue;
        }
        
        while (line_count < (size_t)n && !eof_reached) {

            ssize_t r = read(fd, &ch, 1);
            if (r == 0) {
                eof_reached = 1;
                if (len > 0) {
                    line_count++;
                    if (line_count > capacity) {
                        capacity *= 2;
                        table = realloc(table, capacity * sizeof(lineInfo));
                        if (!table) {
                            perror("Out of memory");
                            close(fd);
                            exit(1);
                        }
                    }
                    table[line_count - 1].offset = offset - len;
                    table[line_count - 1].length = len;
                }
                break;
            }
            if (r < 0) {
                perror("Error while reading file");
                free(table);
                close(fd);
                exit(1);
            }

            offset++;
            len++;

            if (ch == '\n') {
                line_count++;
                if (line_count > capacity) {
                    capacity *= 2;
                    table = realloc(table, capacity * sizeof(lineInfo));
                    if (!table) {
                        perror("Out of memory");
                        close(fd);
                        exit(1);
                    }
                }

                table[line_count - 1].offset = offset - len;
                table[line_count - 1].length = len;
                len = 0;
            }
        }

        if (n > (int)line_count) {
            printf("File has only %zu lines.\n", line_count);
            continue;
        }

        if (lseek(fd, table[n - 1].offset, SEEK_SET) < 0) {
            perror("lseek error");
            free(table);
            close(fd);
            exit(1);
        }

        char *buf = malloc(table[n - 1].length + 1);
        if (!buf) {
            perror("malloc error");
            free(table);
            close(fd);
            exit(1);
        }

        ssize_t r = read(fd, buf, table[n - 1].length);
        if (r < 0) {
            perror("read error");
            free(buf);
            close(fd);
            exit(1);
        }
        buf[table[n - 1].length] = '\0';
        printf("%s\n", buf);
        free(buf);
    }

    free(table);
    close(fd);
    exit(0);
}
