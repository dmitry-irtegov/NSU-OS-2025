#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

int match_pattern(const char *pattern, const char *name) {
    const char *p = pattern;
    const char *n = name;
    const char *star_p = NULL;
    const char *star_n = NULL;

    while(*n) {

        if (*p == '*') {
            star_p = ++p;
            star_n = n;
            continue;
        }

        if (*p == *n || *p == '?') {
            p++;
            n++;
            continue;
        }

        if (star_p) {
            p = star_p;
            n = ++star_n;
            continue;
        }

        return 0;
    }

    while(*p == '*') p++;

    return(*p == '\0');
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pattern>\n", argv[0]);
        return 1;
    }

    if (strchr(argv[1], '/') != NULL) {
        fprintf(stderr, "Error: '/' isn`t allowed in pattern\n");
        return 1;
    }

    DIR *d = opendir(".");
    if (!d) {
        perror("Cannot open directory");
        return 1;
    }

    struct dirent *ent;
    int matched = 0;
    errno = 0;

    while (1) {
        ent = readdir(d);

        if (ent == NULL) {
            if (errno != 0) {
                perror("readdir");
                closedir(d);
                return 1;
            }
            break;
        }

        if (match_pattern(argv[1], ent->d_name)) {
            printf("%s\n", ent->d_name);
            matched = 1;
        }

        errno = 0;
    }

    if (!matched) {
        printf("%s\n", argv[1]);
    }

    if (closedir(d) == -1) {
        perror("Error of closing directory");
        return 1;
    }

    return 0;
}
