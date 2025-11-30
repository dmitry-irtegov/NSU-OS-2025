#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void closeAll(FILE* fp, FILE* pipe_fp, char* str) {
    free(str);
    fclose(fp);
    if (pclose(pipe_fp) == -1) {
        perror("pclose error");
        exit(EXIT_FAILURE);
    }
}

#define COMMAND_WC "wc -l"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("incorrect count of argument: need 1, get %d\n", argc - 1);
        exit(EXIT_FAILURE);
    }
    size_t ln = 0;
    char* str = NULL;

    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror("fopen error");
        exit(EXIT_FAILURE);
    }
    FILE* pipe_fp = popen(COMMAND_WC, "w");
    if (pipe_fp == NULL) {
        perror("popen error");
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    while (getline(&str, &ln, fp) != -1) {
        if (str[0] == '\n') {
            if (fputc(str[0], pipe_fp) == -1) {
                perror("fputc error");
                closeAll(fp, pipe_fp, str);
                exit(EXIT_FAILURE);
            }
        }
    }

    closeAll(fp, pipe_fp, str);
    exit(EXIT_SUCCESS);
}