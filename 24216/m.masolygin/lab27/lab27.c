#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void end(FILE* fp, FILE* pipe_fp, char* str, int code) {
    fclose(fp);
    if (pclose(pipe_fp) == -1) {
        perror("pclose");
        exit(1);
    }
    free(str);
    exit(code);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror("fopen");
        return 1;
    }

    FILE* pipe_fp = popen("wc -l", "w");
    if (pipe_fp == NULL) {
        perror("popen");
        fclose(fp);
        return 1;
    }

    char* str = NULL;
    size_t len = 0;

    while (getline(&str, &len, fp) != -1) {
        if (str[0] == '\n') {
            if (fputc(str[0], pipe_fp) == -1) {
                perror("fputc");
                end(fp, pipe_fp, str, 1);
            }
        }
    }

    end(fp, pipe_fp, str, 0);

    return 0;
}
