#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#define DEFAULT_CAP 10
#define BUFFER_SIZE 4096
#define TIMEOUT_SECONDS 5

typedef struct Row {
    off_t offset;
    off_t length;
} Row;

typedef struct Table {
    Row *rows;
    int size;
    int capacity;
} Table;

int flag;

int createTable(Table *table) {
    table->rows = malloc(DEFAULT_CAP * sizeof(Row));
    if (!table->rows) {
        perror("malloc error");
        return -1;
    }

    table->size = 0;
    table->capacity = DEFAULT_CAP;
    return 0;
}

int addRow(Table *table, off_t offset, off_t length) {
    if (table->size >= table->capacity) {
        table->capacity *= 2;
        table->rows = realloc(table->rows, table->capacity * sizeof(Row));
        if (!table->rows) {
            perror("Realloc error.");
            return -1;
        }
    }
    table->rows[table->size].offset = offset;
    table->rows[table->size].length = length;
    table->size++;
    return 0;
}

int fillTable(Table *table, int fd) {
    char buffer[BUFFER_SIZE];
    off_t totalBytes = 0;
    off_t lineStart = 0;
    ssize_t bytesRead;

    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek error");
        return -1;
    }

    while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0)
    {
        for (ssize_t i = 0; i < bytesRead; i++)
        {
            if (buffer[i] == '\n') {
                off_t lineLength = (i + totalBytes) - lineStart;
                if (addRow(table, lineStart, lineLength) == -1) {
                    return -1;
                }
                lineStart = totalBytes + i + 1;
            }
        }
        totalBytes += bytesRead;
    }

    if (totalBytes > lineStart) {
        off_t lineLength = totalBytes - lineStart;
        if (addRow(table, lineStart, lineLength) == -1) {
            return -1;
        }
    }

    return 0;
}

int linePrint(Table *table, int fd, int lineNumber) {
    off_t lineLength = table->rows[lineNumber - 1].length;
    if (lineLength < 0)
    {
        fprintf(stderr, "Negative line length\n");
        return -1;
    }

    if (lseek(fd, table->rows[lineNumber - 1].offset, SEEK_SET) == -1) {
        perror("lseek error.");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    off_t remaining = lineLength;

    while (remaining > 0) {
        off_t chunkSize = remaining < (off_t)sizeof(buffer) ? remaining : (off_t)sizeof(buffer);

        bytesRead = read(fd, buffer, chunkSize);
        if (bytesRead == -1) {
            perror("File reading error");
            return -1;
        }

        if (bytesRead == 0) {
            break;
        }

        if (write(STDOUT_FILENO, buffer, bytesRead) == -1) {
            perror("Write error");
            return -1;
        }

        remaining -= bytesRead;
    }
    printf("\n");
    return 0;
}

void freeTable(Table *table) {
    if (table->rows) {
        free(table->rows);
    }
}

void sigcatch() {
    flag = 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Unexpected count of arguments\n");
        return -1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("File opening error");
        return -1;
    }

    Table table;
    if (createTable(&table) == -1) {
        close(fd);
        return -1;
    }

    if (fillTable(&table, fd) == -1) {
        freeTable(&table);
        close(fd);
        return -1;
    }

    printf("Total lines in file: %d\n", table.size);

    struct sigaction sa;
    sa.sa_handler = sigcatch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        close(fd);
        freeTable(&table);
        return -1;
    }

    int lineNumber;
    setbuf(stdin, NULL);

    do {
        printf("Enter line number: ");

        flag = 0;
        alarm(TIMEOUT_SECONDS);
        int result = scanf("%d", &lineNumber);
        alarm(0);


        if (flag) {
            printf("\n");
            for (int i = 1; i <= table.size; i++) {
                if (linePrint(&table, fd, i) == -1) {
                    freeTable(&table);
                    close(fd);
                    return -1;
                }
            }
            break;
        }

        if (result != 1) {
            if (result == 0) {
                printf("Line number was not recognized.\n");
                scanf("%*[^\n]");
                continue;
            } else if (feof(stdin)) {
                printf("End of input reached\n");
                close(fd);
                freeTable(&table);
                return -1;
            } else {
                perror("Input error");
                close(fd);
                freeTable(&table);
                return -1;
            }
        }

        if (lineNumber == 0) {
            break;
        } else if (lineNumber < 1 || lineNumber > table.size) {
           printf("Invalid line number! Total lines in file: %d\n", table.size);
           continue;
        }
        if (linePrint(&table, fd, lineNumber) == -1) {
            freeTable(&table);
            close(fd);
            return -1;
        }
    } while (1);

    freeTable(&table);
    close(fd);
    return 0;
}
