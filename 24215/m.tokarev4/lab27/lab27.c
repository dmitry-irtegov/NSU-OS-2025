#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    FILE* fp;
    int emptylines;
    
    if (argc != 2) {
        printf("Использование: %s <файл>\n", argv[0]);
        return 1;
    }


    char cmd[256];
    snprintf(cmd, sizeof(cmd), "grep '^$' '%s' | wc -l", argv[1]);

    if ((fp = popen(cmd, "r")) == NULL) {
        return 1;
    }
    fscanf(fp, "%d", &emptylines);
    pclose(fp);

    printf("Пустых строк: %d\n", emptylines);
    return 0;
}
