#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <fnmatch.h>

int found = 0;

void walk_dir(const char *base, const char *pattern)
{
    DIR *dir = opendir(base);
    if (!dir) {
        perror(base);
        return;
    }

    struct dirent *entry;
    while (1) {
        errno = 0;
        entry = readdir(dir);

        if (entry == NULL) {
            if (errno != 0) {
                if (errno == EOVERFLOW) {
                    fprintf(stderr, "EOVERFLOW in directory. Please, recompile with -D_FILE_OFFSET_BITS=64\n");
                } else {
                    perror(base);
                }
            }
            break;
        }

        const char *name = entry->d_name;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        char fullpath[PATH_MAX];

        if (strcmp(base, ".") == 0) {
            snprintf(fullpath, sizeof(fullpath), "%s", name);
        } else {
            snprintf(fullpath, sizeof(fullpath), "%s/%s", base, name);
        }

        struct stat st;
        if (stat(fullpath, &st) != 0) {
            perror(fullpath);
            continue;
        }

        if (S_ISREG(st.st_mode) && fnmatch(pattern, fullpath, 0) == 0) {
            printf("%s\n", fullpath);
            found = 1;
        } else if (S_ISDIR(st.st_mode)) {
            walk_dir(fullpath, pattern);
        }
    }

    if (closedir(dir) != 0) {
        perror(base);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s '<pattern>'\n", argv[0]);
        return 1;
    }

    walk_dir(".", argv[1]);

    if (!found) {
        printf("Your pattern: '%s'\n", argv[1]);
    }

    return 0;
}
