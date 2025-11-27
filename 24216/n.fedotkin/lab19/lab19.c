#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

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

int scan_directory(const char *pattern) {
    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return -1;
    }

    struct dirent *entry;
    int found = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (match_pattern(pattern, entry->d_name)) {
            printf("%s\n", entry->d_name);
            found = 1;
        }
    }

    if (errno) {
        perror("readdir");
        closedir(dir);
        return -1;
    }

    closedir(dir);
    return found;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "To use: %s <template>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strchr(argv[1], '/')) {
        fprintf(stderr, "Error: '/' must not occur in the template!\n");
        exit(EXIT_FAILURE);
    }

    int result = scan_directory(argv[1]);

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