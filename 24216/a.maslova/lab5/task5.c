#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096
#define INIT_CAP 1000

typedef struct { 
    off_t offset, length; 
} LineInfo;

typedef struct { 
    LineInfo *lines; 
    int count, capacity; 
} LineTable;

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
    char buf[BUFFER_SIZE];
    off_t total = 0, start = 0;
    
    lseek(fd, 0, SEEK_SET);
    while (1) {
        ssize_t n = read(fd, buf, BUFFER_SIZE);
        if (n < 0) return -1;
        if(n == 0) break;
        
        for (ssize_t i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                if (add_line(tab, start, total + i - start) < 0) return -1;
                start = total + i + 1;
            }
        }
        total += n;
    }
    if (start < total & add_line(tab, start, total - start) < 0) return -1;

    return 0;
}

int print_line(int fd, LineInfo *info) {
    char *buf = malloc(info->length + 1);
    if (!buf) return -1;
    
    lseek(fd, info->offset, SEEK_SET);
    ssize_t n = read(fd, buf, info->length);
    if (n < 0) {
        free(buf);
        return -1;
    }

    buf[n] = '\0';
    printf("%s\n", buf);
    free(buf);
    return 0;
}

int process_input(int fd, LineTable *tab) {
    int num;
    char input[128];
    
    while (1) {
        printf("Enter line number (0 to exit): ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) return -1;
        if (sscanf(input, "%d", &num) != 1) continue;
        if (num == 0) break;
        if (num < 1 || num > tab->count) {
            printf("Invalid line (1-%d)\n", tab->count);
            continue;
        }
        print_line(fd, &tab->lines[num - 1]);
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
        fprintf(stderr, "Failed to build line table\n");
        close(fd);
        free(table.lines);
        return 1;
    }
    
    printf("Total lines: %d\n", table.count);
    if (process_input(fd, &table) < 0) {
        fprintf(stderr, "Error processing input\n");
        close(fd);
        free(table.lines);
        return 1;
    }
    
    close(fd);
    free(table.lines);
    return 0;
}
