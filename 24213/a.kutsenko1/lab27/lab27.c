#include <stdio.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char** argv) {
    char line[BUFSIZ];
    FILE* inputFile;
    FILE* wc_pipe;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    inputFile = fopen(argv[1], "r");
    if (inputFile == NULL) {
        perror("Error opening file");
        return 1;
    }

    wc_pipe = popen("wc -l", "w");
    if (wc_pipe == NULL) {
        perror("Error creating pipe");
        fclose(inputFile);
        return 1;
    }

    int previous_was_complete = 1;

    while (fgets(line, BUFSIZ, inputFile) != NULL) {
        int len = strlen(line);
        int ends_with_newline = (len > 0 && line[len-1] == '\n');
        int is_empty_line = 1;
        int check_len = ends_with_newline ? len - 1 : len;
        for (int i = 0; i < check_len; i++) {
            if (!isspace((unsigned char)line[i])) {
                is_empty_line = 0;
                break;
            }
        }
        if (is_empty_line && previous_was_complete) {
            fputs("\n", wc_pipe);
        }
        previous_was_complete = ends_with_newline;
    }

    fclose(inputFile);
    pclose(wc_pipe);
    return 0;
}
