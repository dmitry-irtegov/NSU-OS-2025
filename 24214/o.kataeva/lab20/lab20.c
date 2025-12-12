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


int search_dir(const char *dirpath, const char *pattern, int *found) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    char path[4096];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);

        struct stat st;
        if (stat(path, &st) == -1)
            continue;

        if (match_pattern(pattern, path + 2)) 
        {
            printf("%s\n", path + 2);
            *found = 1;
        }

        if (S_ISDIR(st.st_mode)) {
            search_dir(path, pattern, found);
        }
    }

    if (errno) {
        perror("readdir");
        closedir(dir);
        return 1;
    }

    closedir(dir);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <template>\n", argv[0]);
        return 1;
    }

    int found = 0;
    if (!search_dir(".", argv[1], &found)) {
        return 1;
    }

    if (!found)
        printf("%s\n", argv[1]);

    return 0;
}
