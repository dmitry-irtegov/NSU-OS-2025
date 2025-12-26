#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <sys/stat.h>
#define DEFAULT_CAP 10
#define BUFFER_SIZE 4096
#define TIMEOUT_SECONDS 5

typedef struct Row {
    size_t offset;
    size_t length;
} Row;

typedef struct Table {
    Row *rows;
    int size;
    int capacity;
} Table;

int flag;
const char *file_data = NULL;
size_t file_size = 0;

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

int addRow(Table *table, size_t offset, size_t length) {
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

int fillTable(Table *table, const char *data, size_t size) {
    size_t lineStart = 0;
    for (size_t i = 0; i < size; i++) {
        if (data[i] == '\n') {
            if (addRow(table, lineStart, i - lineStart) == -1) {
                return -1;
            }
            lineStart = i + 1;
        }
    }
    if (lineStart < size) {
        if (addRow(table, lineStart, size - lineStart) == -1) {
            return -1;
        }
    }
    return 0;
}

int linePrint(Table *table, int lineNumber) {
    size_t offset = table->rows[lineNumber - 1].offset;
    size_t length = table->rows[lineNumber - 1].length;

    if (write(STDOUT_FILENO, file_data + offset, length) == -1) {
        perror("Write error");
        return -1;
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

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat error");
        close(fd);
        return -1;
    }

    file_size = st.st_size;
    if (file_size == 0) {
        printf("File is empty\n");
        close(fd);
        return 0;
    }

    file_data = (const char*)mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("mmap error");
        close(fd);
        return -1;
    }
    close(fd);

    Table table;
    if (createTable(&table) == -1) {
        munmap((void*)file_data, file_size);
        return -1;
    }

    if (fillTable(&table, file_data, file_size) == -1) {
        freeTable(&table);
        munmap((void*)file_data, file_size);
        return -1;
    }

    printf("Total lines in file: %d\n", table.size);

    struct sigaction sa;
    sa.sa_handler = sigcatch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1) {
        perror("sigaction");
        freeTable(&table);
        munmap((void*)file_data, file_size);
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
            if (write(STDOUT_FILENO, file_data, file_size) == -1)
            {
                perror("Write error");
                return -1;
            }

            break;
        }

        if (result != 1) {
            if (result == 0) {
                printf("Invalid input. Please enter a number.\n");
                scanf("%*[^\n]");
                continue;
            } else if (feof(stdin)) {
                printf("End of input reached\n");
                freeTable(&table);
                munmap((void*)file_data, file_size);
                return -1;
            } else {
                perror("Input error");
                freeTable(&table);
                munmap((void*)file_data, file_size);
                return -1;
            }
        }

        if (lineNumber == 0) {
            break;
        } else if (lineNumber < 1 || lineNumber > table.size) {
           printf("Invalid line number! Total lines in file: %d\n", table.size);
           continue;
        }
        if (linePrint(&table, lineNumber) == -1) {
            freeTable(&table);
            munmap((void*)file_data, file_size);
            return -1;
        }
    } while (1);

    freeTable(&table);
    munmap((void*)file_data, file_size);
    return 0;
}
