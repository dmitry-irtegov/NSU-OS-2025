#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <errno.h>

bool match(const char *pattern, const char *filename) {
    const char *pattern_ptr = pattern;
    const char *file_ptr = filename;
    const char *backtrack_pattern = NULL;
    const char *backtrack_file = NULL;

    while (*file_ptr != '\0') {
        if (*pattern_ptr == '*') {
            backtrack_pattern = ++pattern_ptr;
            backtrack_file = file_ptr;
        } else if (*pattern_ptr == '?') {
            if (*file_ptr == '/') {
                return false;
            }
            pattern_ptr++;
            file_ptr++;
        } else if (*pattern_ptr == *file_ptr) {
            pattern_ptr++;
            file_ptr++;
        } else if (backtrack_pattern != NULL) {
            pattern_ptr = backtrack_pattern;
            file_ptr = ++backtrack_file;
        } else {
            return false;
        }
    }

    while (*pattern_ptr == '*') {
        pattern_ptr++;
    }

    return *pattern_ptr == '\0';
}

bool is_pattern_valid(const char *pattern) {
    for (const char *p = pattern; *p != '\0'; p++) {
        if (*p == '/') {
            fprintf(stderr, "Ошибка: шаблон не должен содержать символ '/'\n");
            return false;
        }
    }
    return true;
}

int main(void) {
    char pattern[256];
    
    printf("Введите шаблон имени файла: ");
    if (fgets(pattern, sizeof(pattern), stdin) == NULL) {
        fprintf(stderr, "Ошибка чтения ввода: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    size_t len = strlen(pattern);
    if (len > 0 && pattern[len - 1] == '\n') {
        pattern[len - 1] = '\0';
    }

    if (pattern[0] == '\0') {
        fprintf(stderr, "Ошибка: шаблон не может быть пустым\n");
        return EXIT_FAILURE;
    }

    if (!is_pattern_valid(pattern)) {
        return EXIT_FAILURE;
    }

    DIR *dir = opendir(".");
    if (dir == NULL) {
        fprintf(stderr, "Ошибка открытия каталога: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    bool found_match = false;
    errno = 0;

    printf("Файлы, соответствующие шаблону '%s':\n", pattern);
    
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (match(pattern, entry->d_name)) {
            printf("  %s\n", entry->d_name);    
            found_match = true;
        }
    }

    if (errno != 0) {
        fprintf(stderr, "Ошибка чтения каталога: %s\n", strerror(errno));
        closedir(dir);
        return EXIT_FAILURE;
    }

    if (!found_match) {
        printf("Нет файлов, соответствующих шаблону.\n %s\n", pattern);
    }

    if (closedir(dir) != 0) {
        fprintf(stderr, "Ошибка закрытия каталога: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}