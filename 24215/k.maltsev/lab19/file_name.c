#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

int pattern_match(const char *pattern, const char *string) {
    if (*pattern == '\0') {
        return *string == '\0';
    }

    if (*pattern == '*') {
        if (pattern_match(pattern + 1, string)) {
            return 1;
        }
        if (*string != '\0' && pattern_match(pattern, string + 1)) {
            return 1;
        }
        return 0;
    }

    if (*pattern == '?') {
        if (*string == '\0') {
            return 0;
        }
        return pattern_match(pattern + 1, string + 1);
    }

    if (*pattern == *string) {
        return pattern_match(pattern + 1, string + 1);
    }

    return 0;
}

int main() {
    char pattern[1024];
    DIR *d;
    struct dirent *dir;
    int found_flag = 0;

    printf("Enter the file name pattern: ");
    if (fgets(pattern, sizeof(pattern), stdin) == NULL) {
        return 0;
    }

    size_t len = strlen(pattern);
    if (len > 0 && pattern[len - 1] == '\n') {
        pattern[len - 1] = '\0';
    }

    if (strchr(pattern, '/') != NULL) {
        fprintf(stderr, "Error: The '/' character is not allowed in the pattern.\n");
        return 1;
    }

    d = opendir(".");
    if (d == NULL) {
        perror("opendir");
        return 1;
    }

    while ((dir = readdir(d)) != NULL) {
        if (pattern_match(pattern, dir->d_name)) {
            printf("%s\n", dir->d_name);
            found_flag = 1;
        }
    }
    closedir(d);

    if (!found_flag) {
        printf("%s\n", pattern);
    }

    return 0;
}
