#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 1024

int main()
{
    FILE *pipe = popen("./toup", "w");
    if (pipe == NULL) {
        perror("popen");
        return 1;
    }

    char buf[BUF_SIZE];
    size_t n = fread(buf, 1, sizeof(buf), stdin);

    int exit_val = 0;
    while (n > 0) {
        if (fwrite(buf, 1, n, pipe) != n) {
            perror("fwrite to pipe");
            exit_val = 1;
            break;
        }
        n = fread(buf, 1, sizeof(buf), stdin);
    }

    if (ferror(stdin)) {
        perror("fread");
        exit_val = 1;
    }

    if (pclose(pipe) == -1) {
        perror("pclose");
        return 1;
    }

    return exit_val;
}
