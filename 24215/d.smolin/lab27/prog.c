#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s FILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("fopen");
        return EXIT_FAILURE;
    }

    FILE *wc = popen("wc -l", "w");
    if (!wc) {
        perror("popen");
        fclose(file);
        return EXIT_FAILURE;
    }

    char buf[4096];
    while (fgets(buf, sizeof(buf), file)) {
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] == '\n') {
            buf[len - 1] = '\0';
            len--;
        }
        if (len == 0) {
            fputc('\n', wc);
        }
    }

    fclose(file);
    pclose(wc);
    return 0;
}
