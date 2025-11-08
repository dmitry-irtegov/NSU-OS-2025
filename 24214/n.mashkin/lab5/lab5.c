#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

int main() {
    int fd = open("file.txt", O_RDONLY);
    if (fd == -1) {
        perror("Could not open a file");
        exit(-1);
    }

    int table_size = 10;
    off_t *table = malloc(table_size * sizeof(off_t));
    if (table == NULL) {
        perror("Could not allocate memory for a table");
        close(fd);
        exit(-1);
    }

    char buf[4096];
    int has_read, table_ind = 1, cnt = 1;
    off_t read_count = 0;
    table[0] = 0;
    while ((has_read = read(fd, buf, 4096)) > 0) {
        for (int i = 0; i < has_read; i++) {
            if (buf[i] == '\n') {
                table[table_ind++] = (read_count * 4096) + i + 1;

                if (table_ind == table_size) {
                    table_size *= 2;
                    table = realloc(table, table_size * sizeof(off_t));
                    if (table == NULL) {
                        perror("Could not realloc memory");
                        close(fd);
                        exit(-1);
                    }
                }
            }
        }
        read_count++;
    }
    if (has_read == -1) {
        perror("Error while reading");
        close(fd);
        free(table);
        exit(-1);
    }

    // for (int i = 0; i < table_ind; i++) {
    //     printf("%ld\n", table[i]);
    // }

    if (lseek(fd, (off_t)0, SEEK_SET) == -1) {
        perror("Could not return to the beginning of the file");
        close(fd);
        free(table);
        exit(-1);
    }

    int answer;
    printf("Which line would you like to see?: ");
    if (scanf("%d", &answer) != 1) {
        fprintf(stderr, "Could not read a line number");
        close(fd);
        free(table);
        exit(-1);
    }
    while (answer) {
        if (answer < 0 || answer >= table_ind) {
            printf("Please, enter a valid line number: ");
            if (scanf("%d", &answer) != 1) {
                fprintf(stderr, "Could not read a line number");
                close(fd);
                free(table);
                exit(-1);
            }
            continue;
        }

        if (lseek(fd, table[answer - 1], SEEK_SET) == -1) {
            perror("Error while finding the desired line");
            close(fd);
            free(table);
            exit(-1);
        }
        while ((has_read = read(fd, buf, 4096)) > 0) {
            int line_end = 0;
            for (int i = 0; i < has_read; i++) {
                printf("%c", buf[i]);
                if (buf[i] == '\n') {
                    line_end = 1;
                    break;
                }
            }
            if (line_end) {
                break;
            }
        }
        if (has_read == -1) {
            perror("Error while reading a file");
            close(fd);
            free(table);
            exit(-1);
        }

        printf("Which line would you like to see?: ");
        if (scanf("%d", &answer) != 1) {
            fprintf(stderr, "Could not read a line number");
            close(fd);
            free(table);
            exit(-1);
        }
    }

    exit(0);
}

