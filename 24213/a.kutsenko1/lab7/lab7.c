#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/select.h>

int main(int argc, char *argv[]) {
    setbuf(stdin, NULL);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    struct stat file_info;
    if (fstat(fd, &file_info) == -1) {
        perror("Error getting file information");
        close(fd);
        return 1;
    }

    size_t file_size = file_info.st_size;

    char *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("Error mapping file to memory");
        close(fd);
        return 1;
    }

    close(fd);

    int line_counter = 0;
    for (size_t i = 0; i < file_size; i++) {
        if (file_data[i] == '\n') {
            line_counter++;
        }
    }

    if (file_size > 0 && file_data[file_size - 1] != '\n') {
        line_counter++;
    }

    size_t *offsets = malloc(line_counter * sizeof(size_t));
    size_t *lengths = malloc(line_counter * sizeof(size_t));

    if (!offsets || !lengths) {
        perror("Error allocating memory");
        free(offsets);
        free(lengths);
        munmap(file_data, file_size);
        return 1;
    }

    int current_line = 0;
    size_t line_start = 0;
    for (size_t i = 0; i < file_size; i++) {
        if (file_data[i] == '\n') {
            offsets[current_line] = line_start;
            lengths[current_line] = i - line_start;
            line_start = i + 1;
            current_line++;
        }
    }

    if (line_start < file_size) {
        offsets[current_line] = line_start;
        lengths[current_line] = file_size - line_start;
    }
    
    while (1) {
        printf("Enter line number (0 to exit): \n");

        fd_set read_fds;
        struct timeval timeout;
        
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        
        int ready = select(STDIN_FILENO + 1, &read_fds, NULL, NULL, &timeout);
        
        if (ready == -1) {
            perror("select error");
            break;
        } 
        else if (ready == 0) {
            printf("\nTimeout! 5 seconds passed. Printing entire file:\n");
            for (int j = 0; j < line_counter; j++) {
                printf("%.*s\n", (int)lengths[j], file_data + offsets[j]);
            }
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            int line_number;
            int scanf_result = scanf("%d", &line_number);
            if (scanf_result == EOF) {
                if (feof(stdin)) {
                    printf("\nEOF reached. Exiting...\n");
                }
                else if (ferror(stdin)) {
                    perror("Error reading input");
                }
                break;
            }
            else if (scanf_result == 0) {
                printf("Invalid input. Please enter a number.\n");
                scanf("%*s");
                continue;
            }

            if (line_number == 0) {
                printf("Exiting...\n");
                break;
            }

            if (line_number < 1 || line_number > line_counter) {
                printf("Invalid line number. File has %d lines.\n", line_counter);
                continue;
            }

            int idx = line_number - 1;
            printf("Line %d: %.*s\n\n", line_number, (int)lengths[idx], file_data + offsets[idx]);
        }
    }
    free(offsets);
    free(lengths);
    munmap(file_data, file_size);

    printf("Program finished.\n");
    return 0;
}
