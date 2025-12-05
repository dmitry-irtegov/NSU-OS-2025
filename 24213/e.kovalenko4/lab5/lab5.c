#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

typedef struct {
    off_t offset;
    off_t length;
} lineInfo;

void clear_stdin() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

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

    size_t line_count = 0;
    off_t offset = 0, len = 0;
    char ch;
    while (1) {
        ssize_t r = read(fd, &ch, 1);

        if (r == 0) {
            if (len > 0) {
                table[line_count].offset = offset - len;
                table[line_count].length = len;
                line_count++;
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
            if (line_count >= capacity - 1) {
                capacity *= 2;
                table = realloc(table, capacity * sizeof(lineInfo));
                if (!table) {
                    perror("Out of memory");
                    close(fd);
                    exit(1);
                }
            }
            table[line_count].offset = offset - len;
            table[line_count].length = len;
            line_count++;
            len = 0;
        }
    }

    int n;
    while (1) {
        printf("Enter line number (0 - exit): ");
        if (scanf("%d", &n) != 1) {
            printf("Input is not a valid number.\n");
            clear_stdin();
            continue;
        }
        if (n == 0) break;
        if (n < 0) {
            printf("Input must be positive\n");
            continue;
        }
        if ((size_t)n > line_count) {
            printf("File has only %zu lines.\n", line_count);
            continue;
        }

        offset = table[n - 1].offset;
        len = table[n - 1].length;

        if (lseek(fd, offset, SEEK_SET) < 0) {
            perror("lseek error");
            free(table);
            close(fd);
            exit(1);
        }

        char buf[4096];
        off_t remaining = len;
        while (remaining > 0) {
            ssize_t to_read = remaining < (off_t)sizeof(buf)-1 ? remaining : (off_t)sizeof(buf)-1;
            ssize_t r = read(fd, buf, to_read);
            if (r <= 0) {
                perror("read error");
                free(table);
                close(fd);
                exit(1);
            }
            buf[r] = '\0';
            printf("%.*s", (int)r, buf);
            remaining -= r;
        }
    }

    free(table);
    close(fd);
    exit(0);
}
