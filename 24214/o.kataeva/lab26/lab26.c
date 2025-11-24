#include <stdio.h>

#define BUF_SIZE 256

int main(void) {
    const char text[] = "a A df Fd";

    FILE *pipe = popen("tr 'a-z' 'A-Z'", "w");
    if (!pipe) {
        perror("popen failed");
        return 1;
    }

    if (fprintf(pipe, "%s", text) < 0) {
        perror("write failed");
        pclose(pipe);
        return 1;
    }

    if (pclose(pipe) == -1) {
        perror("pclose");
        return 1;
    }

    return 0;
}
