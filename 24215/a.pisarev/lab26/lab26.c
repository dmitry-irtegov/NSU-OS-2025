#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *fp;


    fp = popen("tr 'a-z' 'A-Z'", "w");
    if (fp == NULL) {
        perror("popen");
        return 1;
    }

    fprintf(fp, "I love ice cream!\n");

    if (pclose(fp) == -1) {
        perror("pclose");
        return 1;
    }

    return 0;
}
