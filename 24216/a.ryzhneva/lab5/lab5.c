#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "use: %s <filename>\n", argv[0]);
        exit(1);
    }

    const char *filename = argv[1];
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        exit(1);
    }

    LineInfo *table = NULL;
    size_t table_capacity = 0;
    size_t line_count = 0;
    
    off_t current_line_start = 0;
    off_t current_pos = 0;
    char buffer[1024];
    int bytes_read;

    lseek(fd, 0L, SEEK_SET);

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            current_pos++;

            if (buffer[i] == '\n') {
                if (line_count >= table_capacity) {
                    table_capacity = (table_capacity == 0) ? 128 : table_capacity * 2;
                    LineInfo *new_table = realloc(table, table_capacity * sizeof(LineInfo));
                    if (!new_table) {
                        perror("realloc failed");
                        close(fd);
                        free(table);
                        exit(1);
                    }
                    table = new_table;
                }

                table[line_count].offset = current_line_start;
                table[line_count].length = current_pos - current_line_start;
                
                line_count++;
                current_line_start = current_pos;
            }
        }
    }

    if (bytes_read == -1) {
        perror("read failed");
        close(fd);
        free(table);
        exit(1);
    }

    printf("count line: %lu\n", line_count);
    for (size_t i = 0; i < line_count; i++) {
        printf("%lu\t%ld\t\t%lu\n", i + 1, (long)table[i].offset, table[i].length);
    }

    long line_num;
    char *line_buffer = NULL;

    while (1) {
        printf("Enter the line number (0 for exit): ");
        if (scanf("%ld", &line_num) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

        if (line_num == 0) {
            break; 
        }

        if (line_num < 1 || (size_t)line_num > line_count) {
            printf("Wrong count of line. Correct range: 1 - %lu\n", line_count);
            continue;
        }

        size_t idx = line_num - 1; 

        if (lseek(fd, table[idx].offset, SEEK_SET) == -1) {
            perror("lseek error");
            break;
        }

        line_buffer = malloc(table[idx].length + 1);
        if (!line_buffer) {
            perror("malloc error");
            break;
        }

        ssize_t rb = read(fd, line_buffer, table[idx].length);

        if (rb == -1) {
            perror("read error");
            free(line_buffer);
            break;
        }
        if ((size_t)rb != table[idx].length) {
            fprintf(stderr, "\n");
            free(line_buffer);
            break;
        }

        line_buffer[rb] = '\0';
        printf("%s", line_buffer);

        free(line_buffer);
    }

    printf("complection of work\n");
    free(table);
    close(fd);
    return 0;
}