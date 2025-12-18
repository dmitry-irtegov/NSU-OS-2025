#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
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

void search_in_directory(const char *dir_path, const char *pattern, const char *base_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        fprintf(stderr, "Ошибка открытия каталога '%s': %s\n", dir_path, strerror(errno));
        return;
    }

    struct dirent *entry;
    char full_path[PATH_MAX + 1];
    char relative_path[PATH_MAX + 1];
    struct stat statbuf;
    
    errno = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        if (strcmp(base_path, ".") == 0) {
            snprintf(relative_path, sizeof(relative_path), "%s", entry->d_name);
        } else {
            snprintf(relative_path, sizeof(relative_path), "%s/%s", base_path, entry->d_name);
        }

        if (lstat(full_path, &statbuf) == -1) {
            fprintf(stderr, "Ошибка получения информации о файле '%s': %s\n", 
                    full_path, strerror(errno));
            continue;
        }

        if (match(pattern, relative_path)) {
            printf("  %s\n", relative_path);
        }

        if (S_ISDIR(statbuf.st_mode)) {
            if (strcmp(base_path, ".") == 0) {
                search_in_directory(full_path, pattern, entry->d_name);
            } else {
                char new_base_path[1024];
                snprintf(new_base_path, sizeof(new_base_path), "%s/%s", base_path, entry->d_name);
                search_in_directory(full_path, pattern, new_base_path);
            }
        }
    }

    if (errno != 0) {
        fprintf(stderr, "Ошибка чтения каталога '%s': %s\n", dir_path, strerror(errno));
    }

    if (closedir(dir) != 0) {
        fprintf(stderr, "Ошибка закрытия каталога '%s': %s\n", dir_path, strerror(errno));
    }
}


int main(void) {
    char pattern[NAME_MAX + 1];
    
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

    if (pattern[0] == '/') {
        fprintf(stderr, "Ошибка: шаблон должен быть относительным путем\n");
        return EXIT_FAILURE;
    }

    printf("Файлы, соответствующие шаблону '%s':\n", pattern);
    
    search_in_directory(".", pattern, ".");

    printf("Шаблон: %s\n", pattern);

    return EXIT_SUCCESS;
}