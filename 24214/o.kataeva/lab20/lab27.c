#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){
    char* str = NULL;
    size_t len = 0;
    FILE *file, *output;

    if (argc < 2) {
        fprintf(stderr, "No arguments\n");
        return 1;
    }

    file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening the file");
        return 1;
    }

    output = popen("wc -l", "w");
    
    if (!output) {
        perror("Can't open pipe");
        fclose(file);
        return 1;
    }

    while (getline(&str, &len, file) != -1) {
        if (str[0] == '\n' || str[0] == '\r') {
            fputs("\n", output);
        }
    }

    pclose(output);
    fclose(file);

    return 0;
}
