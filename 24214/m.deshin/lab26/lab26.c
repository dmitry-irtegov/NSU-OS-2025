#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

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

    return 0;
}
