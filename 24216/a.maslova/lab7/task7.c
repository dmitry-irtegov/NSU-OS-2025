#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define BUFFER_SIZE 4096
#define INIT_CAP 1000
#define TIMEOUT 5

typedef struct { 
    off_t offset, length; 
} LineInfo;

typedef struct { 
    LineInfo *lines; 
    int count, capacity; 
} LineTable;

volatile sig_atomic_t timeout = 0;
static char *file_map = NULL;
static size_t file_size = 0;

void alarm_handler() {
    timeout = 1;
}

int init_table(LineTable *tab) {
    tab->capacity = INIT_CAP;
    tab->count = 0;
    tab->lines = malloc(tab->capacity * sizeof(LineInfo));
    if (!tab->lines) {
        return -1;
    }
    return 0;
}

int add_line(LineTable *tab, off_t offset, off_t length) {
    if (tab->count >= tab->capacity) {
        int new_cap = tab->capacity * 2;
        LineInfo *new_lines = realloc(tab->lines, new_cap * sizeof(LineInfo));
        if (!new_lines) return -1;
        tab->lines = new_lines;
        tab->capacity = new_cap;
    }
    tab->lines[tab->count].offset = offset;
    tab->lines[tab->count].length = length;
    tab->count++;
    return 0;
}

int build_table(int fd, LineTable *tab) {
    struct stat st;
    if (fstat(fd, &st) < 0) return -1;
    
    file_size = st.st_size;
    if (file_size == 0) return 0;
    
    file_map = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_map == MAP_FAILED) return -1;
    
    off_t start = 0;
    for (size_t i = 0; i < file_size; i++) {
        if (file_map[i] == '\n') {
            if (add_line(tab, start, (off_t)(i - start)) < 0) return -1;
            start = (off_t)(i + 1);
        }
    }
    
    if ((size_t)start < file_size && add_line(tab, start, (off_t)(file_size - start)) < 0) 
        return -1;
    
    return 0;
}

int print_line(int fd __attribute__((unused)), LineInfo *info) {
    if ((size_t)info->offset >= file_size) return -1;
    
    fwrite(file_map + info->offset, 1, (size_t)info->length, stdout);
    printf("\n");
    return 0;
}

int print_entire_file(int fd __attribute__((unused))) {
    if (file_size > 0) {
        fwrite(file_map, 1, file_size, stdout);
    }
    return 0;
}

int process_input(int fd, LineTable *tab) {
    int num;
    char input[128];

    struct sigaction sa;
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 
    sigaction(SIGALRM, &sa, NULL);

    alarm(TIMEOUT);

    while (!timeout) {

        printf("Enter line number (0 to exit): ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            if (timeout) {
                print_entire_file(fd);
                return 0;
            }
            return -1;
        }

        alarm(0);

        if (timeout) {
            print_entire_file(fd);
            return 0;
        }

        if (sscanf(input, "%d", &num) != 1) {
            alarm(TIMEOUT);
            continue;
        }

        if (num == 0) break;

        if (num < 1 || num > tab->count) {
            printf("Invalid line (1-%d)\n", tab->count);
        } else {
            print_line(fd, &tab->lines[num - 1]);
        }

        alarm(TIMEOUT);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }
    
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }
    
    LineTable table = {0};
    if (init_table(&table) < 0 || build_table(fd, &table) < 0) {
        perror("Failed to build line table");
        close(fd);
        free(table.lines);
        return 1;
    }
    
    printf("Total lines: %d\n", table.count);
    if (process_input(fd, &table) < 0) {
        perror("Error processing input");
        close(fd);
        if (file_map != NULL && file_map != MAP_FAILED) {
            munmap(file_map, file_size);
        }
        free(table.lines);
        return 1;
    }
    
    close(fd);
    if (file_map != NULL && file_map != MAP_FAILED) {
        munmap(file_map, file_size);
    }
    free(table.lines);
    return 0;
}