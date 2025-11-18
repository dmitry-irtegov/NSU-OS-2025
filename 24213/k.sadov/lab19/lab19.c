#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>

#define LEN 256

int match(const char *p, const char *s) {
    while (*p) {
        if (*p == '*') {
            p++;

            if (*p == '\0')
                return 1; 

            while (*s) {
                if (match(p, s))
                    return 1;
                s++;
            }
            return 0;
        }
        else if (*p == '?') {
            if (*s == '\0')
                return 0;
            p++;
            s++;
        }
        else {
            if (*p != *s)
                return 0;
            p++;
            s++;
        }
    }
    return *s == '\0';
}

int main(void) {
    char pattern[LEN];

    printf("Enter pattern: ");

    if (fgets(pattern, sizeof(pattern), stdin) == NULL) {
        perror("Input error");
        exit(EXIT_FAILURE);
    }

    size_t len = strlen(pattern);
    if (len > 0 && pattern[len - 1] == '\n') {
        pattern[len - 1] = '\0';
    }

    if (strchr(pattern, '/') != NULL) {
        fprintf(stderr, "Error: '/' isn't allowed in pattern\n");
        exit(EXIT_FAILURE);
    }

    DIR *d = opendir(".");
    if (d == NULL) {
        perror("Cannot open directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *ent;
    int matched = 0;

    while ((ent = readdir(d)) != NULL) {
        if (match(pattern, ent->d_name)) {
            printf("%s\n", ent->d_name);
            matched = 1;
        }
    }

    if (!matched) {
        printf("%s\n", pattern);
    }

    if (closedir(d) == -1) {
        perror("Error closing directory");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
