#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>

char file_type(mode_t mode) {
    if (S_ISDIR(mode)) return 'd';
    if (S_ISREG(mode)) return '-';
    return '?';
}

void print_permissions(mode_t mode) {
    char perms[10];
    perms[0] = (mode & S_IRUSR) ? 'r' : '-';
    perms[1] = (mode & S_IWUSR) ? 'w' : '-';
    perms[2] = (mode & S_IXUSR) ? 'x' : '-';

    perms[3] = (mode & S_IRGRP) ? 'r' : '-';
    perms[4] = (mode & S_IWGRP) ? 'w' : '-';
    perms[5] = (mode & S_IXGRP) ? 'x' : '-';

    perms[6] = (mode & S_IROTH) ? 'r' : '-';
    perms[7] = (mode & S_IWOTH) ? 'w' : '-';
    perms[8] = (mode & S_IXOTH) ? 'x' : '-';

    perms[9] = '\0';
    printf("%s", perms);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> <file2> ...\n", argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (lstat(argv[i], &st) < 0) {
            perror("Can't access file");
            continue;
        }

        char type = file_type(st.st_mode);
        printf("%c", type);

        print_permissions(st.st_mode);

        printf(" %2ju", (uintmax_t)st.st_nlink);

        struct passwd *pw;
        struct group  *gr;

        if ((pw = getpwuid(st.st_uid)) != NULL) {
            printf(" %-8s ", pw->pw_name);
            } else {
                printf(" %-8ju ", (uintmax_t)st.st_uid);
            }
        if ((gr = getgrgid(st.st_gid)) != NULL) {
            printf(" %-8s ", gr->gr_name);
            } else {
                printf(" %-8ju", (uintmax_t)st.st_gid);
            }

        if (S_ISREG(st.st_mode)) {
            printf(" %8jd", (intmax_t)st.st_size);
        } else {
            printf(" %8s", "");
        }

        char *mtime = ctime(&st.st_mtime);
        printf(" %.24s", mtime);

        printf(" %s\n", basename(argv[i]));
    }

    exit(0);
}
