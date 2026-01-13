#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

static int pattern_match(const char *pattern, const char *string) {
    const char *p = pattern;
    const char *s = string;
    const char *star = NULL;
    const char *ss = NULL;

    while (*s != '\0') {
        if (*p == *s || *p == '?') {
            p++;
            s++;
            continue;
        }

        if (*p == '*') {
            star = p;
            p++;
            ss = s;
            continue;
        }

        if (star != NULL) {
            p = star + 1;
            ss++;
            s = ss;
            continue;
        }

        return 0;
    }

    while (*p == '*') p++;
    return *p == '\0';
}

int main(void) {
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
