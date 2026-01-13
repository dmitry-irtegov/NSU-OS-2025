#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEXT 4096
#define MAX_LINE 1024

int main() {
    char text[MAX_TEXT];
    char line[MAX_LINE];
    size_t len = 0;

    printf("Enter text (empty line to finish):\n");

    while (fgets(line, sizeof(line), stdin)) {

        if (strcmp(line, "\n") == 0) {
            break;
        }

        size_t line_len = strlen(line);

        if (len + line_len >= MAX_TEXT) {
            printf("Max text size: 4 KB\n");
            break;
        }

        strcpy(text + len, line);
        len += line_len;
    }

    FILE *fp = popen("./child", "w");
    if (!fp) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    fwrite(text, 1, len, fp);
    pclose(fp);

    return 0;
}