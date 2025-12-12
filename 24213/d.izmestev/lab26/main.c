#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *fp = popen("tr a-z A-Z", "w");
    if (fp == NULL) {
        perror("popen");
        return 1;
    }

    char buf[4096];

    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        if (fputs(buf, fp) == EOF) {
            perror("fputs");
            pclose(fp);
            return 1;
        }
    }

    if (ferror(stdin)) {
        perror("stdin read");
        pclose(fp);
        return 1;
    }

    if (pclose(fp) == -1) {
        perror("pclose");
        return 1;
    }

    return 0;
}
