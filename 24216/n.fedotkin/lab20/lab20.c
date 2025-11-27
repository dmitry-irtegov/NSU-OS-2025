#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

int match_pattern(const char *pattern, const char *name) {
    if (*pattern == '\0' && *name == '\0')
        return 1;

    if (*pattern == '*') {
        return match_pattern(pattern + 1, name) ||
               (*name && match_pattern(pattern, name + 1));
    }

    if (*pattern == '?')
        return *name && match_pattern(pattern + 1, name + 1);

    if (*pattern == *name)
        return match_pattern(pattern + 1, name + 1);

    return 0;
}

void process_entry(const char *path, const char *pattern, int *found) {
    if (match_pattern(pattern, path + 2)) {
        printf("%s\n", path + 2);
        *found = 1;
    }
}

int search_dir(const char *dirpath, const char *pattern) {
    int found = 0;
    DIR *dir;
    if ((dir = opendir(dirpath)) == NULL) {
        perror("Error: opendir");
        return -1;
    }

    struct dirent *entry;
    char path[4096];
    
    while ((entry = readdir(dir)) != NULL) {

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name) >= (int)sizeof(path)) {
            fprintf(stderr, "Path too long: %s/%s\n", dirpath, entry->d_name);
            return -1;
        }

        struct stat st;
        if (stat(path, &st) == -1) {
            perror("Error stating file");
            return -1;
        }
        
        process_entry(path, pattern, &found);

        if (S_ISDIR(st.st_mode)) {
            if (search_dir(path, pattern)) {
                found = 1;
            }
        }
    }

    if (errno) {
        perror("Error: readdir");
        closedir(dir);
        return - 1;
    }

    closedir(dir);
    return found;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <template>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int result = search_dir(".", argv[1]);
    

    switch (result) {
        case -1:
            exit(EXIT_FAILURE);

        case  0:
            printf("%s\n", argv[1]);
            exit(EXIT_SUCCESS);

        default:
            exit(EXIT_SUCCESS);
    }
}