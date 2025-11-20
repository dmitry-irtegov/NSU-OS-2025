#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char get_type_char(const struct stat *st){
    mode_t type = st->st_mode & S_IFMT;

    if (type == S_IFDIR) {
        return 'd';
    } else if (type == S_IFREG) {
        return '-';
    } else {
        return '?';
    }
}

static void print_permissions(const struct stat *st){
    printf("%c%c%c%c%c%c%c%c%c",
           (st->st_mode & S_IRUSR) ? 'r' : '-',
           (st->st_mode & S_IWUSR) ? 'w' : '-',
           (st->st_mode & S_IXUSR) ? 'x' : '-',
           (st->st_mode & S_IRGRP) ? 'r' : '-',
           (st->st_mode & S_IWGRP) ? 'w' : '-',
           (st->st_mode & S_IXGRP) ? 'x' : '-',
           (st->st_mode & S_IROTH) ? 'r' : '-',
           (st->st_mode & S_IWOTH) ? 'w' : '-',
           (st->st_mode & S_IXOTH) ? 'x' : '-');
}

int main(int argc, char **argv){
    for (int i = 1; i < argc; ++i) {
        struct stat st;

        if (lstat(argv[i], &st) == -1) {
            perror(argv[i]);
            continue;
        }

        printf("%c", get_type_char(&st));

        print_permissions(&st);
        printf(" ");

        struct passwd *pw = getpwuid(st.st_uid);
        if (pw == NULL) {
            perror("getpwuid");
            return 1;
        }
        printf("%-16s", pw->pw_name);

        struct group *gr = getgrgid(st.st_gid);
        if (gr == NULL) {
            perror("getgrgid");
            return 1;
        }
        printf("%-16s", gr->gr_name);

        if ((st.st_mode & S_IFMT) == S_IFREG) {
            printf("%-10lld", (long long)st.st_size);
        } else {
            printf("%-10s", "");
        }

        char *timestr = ctime(&st.st_mtime);
        if (timestr == NULL) {
            perror("ctime");
            return 1;
        }
        size_t len = strlen(timestr);
        if (len > 0 && timestr[len - 1] == '\n') {
            timestr[len - 1] = '\0';
        }
        printf("%s ", timestr);

        printf("%s\n", basename(argv[i]));
    }

    return 0;
}
