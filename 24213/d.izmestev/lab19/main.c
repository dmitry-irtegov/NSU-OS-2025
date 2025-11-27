#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <fnmatch.h>

int validate_pattern(char *p)
{
    while (*p) {
        if (*p == '/') {
            return 1;
        }
        p++;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s '<pattern>'\n", argv[0]);
        return 1;
    }

    char *pattern = argv[1];

    if (validate_pattern(pattern)) {
        fprintf(stderr, "Error: '/' is not allowed in pattern\n");
        return 1;
    }

    DIR *dir = opendir(".");
    if (!dir) {
        perror("opendir");
        return 1;
    }

    struct dirent *entry;
    int found = 0;

    while (1) {
        errno = 0;
        entry = readdir(dir);

        if (entry == NULL) {
            if (errno != 0) {
                if (errno == EOVERFLOW) {
                    fprintf(stderr, "EOVERFLOW in directory. Please, recompile with -D_FILE_OFFSET_BITS=64\n");
                } else {
                    perror("readdir");
                }
            }
            break;
        }
        
        char *name = entry->d_name;

        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        struct stat st;
        if (stat(name, &st) != 0) {
            perror("stat");
            continue;
        }

        if (S_ISREG(st.st_mode) && fnmatch(pattern, name, 0) == 0) {
            printf("%s\n", name);
            found = 1;
        }
    }

    if (closedir(dir) != 0) {
        perror("closedir");
        return 1;
    }

    if (!found) {
        printf("Your pattern: '%s'\n", pattern);
    }

    return 0;
}
