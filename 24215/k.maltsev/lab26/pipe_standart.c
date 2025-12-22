#include <stdio.h>
#include <stdlib.h>

#define MSGSIZE 256

int main() {
    FILE *fp;
    char msgout[MSGSIZE] = "Hello, World! I love Osi!";

    printf("Обработанное сообщение:\n");

    if ((fp = popen("tr '[:lower:]' '[:upper:]'", "w")) == NULL) {
        perror("popen");
        exit(1);
    }

    if (fputs(msgout, fp) == EOF) {
        perror("fputs");
        exit(1);
    }

    fputc('\n', fp);

    if (pclose(fp) == -1) {
        perror("pclose");
        exit(1);
    }

    return 0;
}
