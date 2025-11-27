#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE 64
#define MAX_LEN 1024

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "use: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    FILE *fp;
    char *file_name = argv[1];
    char command[MAX_LEN];
    char buffer[BUFF_SIZE];

    int written = snprintf(command, sizeof(command), "grep \"^$\" \"%s\" 2>/dev/null | wc -l", file_name);

    if (written >= (int)sizeof(command) || written < 0) {
        fprintf(stderr, "file_name too long.\n");
        exit(EXIT_FAILURE);
    }

    fp = popen(command, "r");
    if (fp == NULL) {
        perror("popen failed");
        exit(EXIT_FAILURE);
    }

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        printf("empty lines count: %s", buffer);
    } else {
        if (ferror(fp)) {
            perror("fgets failed"); 
            pclose(fp);
            exit(EXIT_FAILURE);
        } else {
            fprintf(stderr, "no output from command.\n");
        }
    }

    if (pclose(fp) == -1) {
        perror("pclose failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}