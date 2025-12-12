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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <template>\n", argv[0]);
        return 1;
    }

    if (strchr(argv[1], '/')) {
        fprintf(stderr, "Error: '/' is not allowed in the template.\n");
        return 1;
    }

    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    int found = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (match_pattern(argv[1], entry->d_name)) {
            printf("%s\n", entry->d_name);
            found = 1;
        }
    }

    if (errno) {
        perror("readdir");
        closedir(dir);
        return 1;
    }

    closedir(dir);

    if (!found)
        printf("%s\n", argv[1]);

    return 0;
}
