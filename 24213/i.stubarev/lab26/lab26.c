#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 4096

int main() {
    FILE *pipe = popen("tr 'a-z' 'A-Z'", "w");
    if (!pipe) {
        perror("popen");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    int rc = 0;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), stdin)) > 0) {
        if (fwrite(buffer, 1, bytes_read, pipe) != bytes_read) {
            perror("write to pipe failed");
            if (pclose(pipe) == -1) {
                perror("pclose");
            }
            return 1;
        }
    }

    if (ferror(stdin)) {
        perror("stdin read error");
        if (pclose(pipe) == -1) {
            perror("pclose");
        }
        return 1;
    }

    if (pclose(pipe) == -1) {
        perror("pclose");
        return 1;
    }

    return 0;
}
