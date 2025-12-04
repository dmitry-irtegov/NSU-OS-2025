#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>

int match(const char *pattern, const char *str) {
    if (*pattern == '\0')
        return *str == '\0';

    if (*pattern == '*') {
        if (match(pattern + 1, str)) {
            return 1;
        }
        if (*str != '\0'){
            return match(pattern, str + 1);
        }
        return 0;
    }

    if (*pattern == '?') {
        if (*str == '\0') {
            return 0;
        }
        return match(pattern + 1, str + 1);
    }

    if (*pattern == *str) {
        return match(pattern + 1, str + 1);
    }

    return 0;
}


int main(void) {
    char pattern[256];

    printf("Enter pattern: ");
    if (!fgets(pattern, sizeof(pattern), stdin)) {
        fprintf(stderr, "Error while entering\n");
        return 1;
    }

    pattern[strcspn(pattern, "\n")] = '\0';

    if (strchr(pattern, '/')) {
        printf("Pattern can not contain '/'\n");
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
        const char *name = entry->d_name;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        if (match(pattern, name)) {
            printf("%s\n", name);
            found = 1;
        }
    }

    closedir(dir);

    if(!found) {
        printf("%s\n", pattern);
    }

    return 0;
}