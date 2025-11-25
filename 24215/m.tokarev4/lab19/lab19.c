#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

int match_pattern(const char* pattern, const char* str) {

    if (*pattern == '\0' && *str == '\0') {
        return 1;
    }

    if (*pattern == '*') {
        while (*pattern == '*') {
            pattern++;
        }

        if (*pattern == '\0') {
            return 1;
        }

        
        if (match_pattern(pattern, str)) {
            return 1;
        }

        while (*str != '\0') {
            if (match_pattern(pattern, str)) {
                return 1;
            }
            str++;
        }
        return 0;
    }

    if (*pattern == '\0' || *str == '\0') {
        return 0;
    }
    
    if (*pattern == '?' || *pattern == *str) {
        return match_pattern(pattern + 1, str + 1);
    }
    return 0;
}

int main() {
    char pattern[256];
    DIR* dir;
    struct dirent* entry;
    int found = 0;
    printf("Введите шаблон имени файла: ");
    if (fgets(pattern, sizeof(pattern), stdin) == NULL) {
        fprintf(stderr, "Ошибка чтения ввода\n");
        return 1;
    }
    char c =  scanf("%c", &c);
    if (c != EOF){
	fprintf(stderr,"Введен слишком большой шаблон\n");
	return 1;
    }
    size_t len = strlen(pattern);
    if (len > 0 && pattern[len - 1] == '\n') {
        pattern[len - 1] = '\0';
    }

    if (strchr(pattern, '/') != NULL) {
        fprintf(stderr, "Ошибка: шаблон не должен содержать символ '/'\n");
        return 1;
    }

    dir = opendir(".");
    if (dir == NULL) {
        perror("Ошибка открытия каталога");
        return 1;
    }

    printf("Файлы, соответствующие шаблону '%s':\n", pattern);

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (match_pattern(pattern, entry->d_name)) {
            printf("%s\n", entry->d_name);
            found = 1;
        }
    }

    if (!found) {
        printf("%s\n", pattern);
    }

    closedir(dir);
    return 0;
}
