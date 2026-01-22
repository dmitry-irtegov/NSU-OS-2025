#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {

    FILE *input, *wc_pipe;
    char buff[BUFSIZ];

    if (argc != 2) {
        fprintf(stderr, "usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    input = fopen(argv[1], "r");
    if (!input) {
        perror("open file");
        exit(EXIT_FAILURE);
    }

    wc_pipe = popen("wc -l", "w");
    if (!wc_pipe) {
        perror("popen");
        fclose(input);
        exit(EXIT_FAILURE);
    }

    int is_new_line = 1;
    while (fgets(buff, BUFSIZ, input)) {
        size_t len = strlen(buff);

        if (is_new_line && len == 1) {
            fputs(buff, wc_pipe);
        }

        is_new_line = (len > 0 && buff[len - 1] == '\n');
    }

    fclose(input);
    if (pclose(wc_pipe) == -1) {
        perror("pclose");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
