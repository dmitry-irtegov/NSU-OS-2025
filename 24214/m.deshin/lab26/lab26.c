#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>

int main() {
    FILE *fp = popen("printf '%s' \"I need to pass lab26!\"", "r");
    if (!fp) {
        perror("popen failed");
        return 1;
    }

    int c;
    while ((c = fgetc(fp)) != EOF) {
        putchar(toupper((unsigned char)c));
    }
    putchar('\n');

    int stat = pclose(fp);
    if (stat == -1) {
        perror("pclose failed");
        return 1;
    }

    if (WIFEXITED(stat)) {
        if (WEXITSTATUS(stat) != 0) {
            fprintf(stderr, "child didn't terminate normally");
        }
    } else {
        fprintf(stderr, "child crashed");
    }

    return 0;
}
