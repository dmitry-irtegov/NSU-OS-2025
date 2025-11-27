#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *fp = popen("tr 'a-z' 'A-Z'", "w");
    if (!fp) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    fputs("Helloo, world!\n", fp);

    if (pclose(fp) == -1) {
        perror("pclose");
        exit(EXIT_FAILURE);
    }

    return 0;
}
