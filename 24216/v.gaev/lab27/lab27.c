#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h> 

int main(int argc, char *argv[]) {
    FILE *fp[2];     
    FILE *infile;     
    char buffer[1024];
    char result[1024];

    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (p2open("wc -l", fp) == -1) {
        perror("p2open failed");
        exit(EXIT_FAILURE);
    }

    infile = fopen(argv[1], "r");
    if (infile == NULL) {
        perror("Не удалось открыть файл");
        p2close(fp);
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, sizeof(buffer), infile) != NULL) {
        if (strcmp(buffer, "\n") == 0) {
            fprintf(fp[0], "\n");
        }
    }

    fclose(infile);
    fclose(fp[0]);
    while (fgets(result, sizeof(result), fp[1]) != NULL) {
        printf("Количество пустых строк: %s", result);
    }

    p2close(fp);

    return 0;
}
