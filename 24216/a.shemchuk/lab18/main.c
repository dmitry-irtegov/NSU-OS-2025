#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        struct stat stat;

        if (lstat(argv[i], &stat) == -1) {
            perror("Something went wrong with lstat");
            continue;
        }
        switch (stat.st_mode & S_IFMT)
        {
        case S_IFDIR:
            printf("d");
            break;
        case S_IFREG:
            printf("-");
            break;
        default:
            printf("?");
            break;
        }

        printf("%c%c%c%c%c%c%c%c%c",
            stat.st_mode & S_IRUSR ? 'r' : '-',
            stat.st_mode & S_IWUSR ? 'w' : '-',
            stat.st_mode & S_IXUSR ? 'x' : '-',
            stat.st_mode & S_IRGRP ? 'r' : '-',
            stat.st_mode & S_IWGRP ? 'w' : '-',
            stat.st_mode & S_IXGRP ? 'x' : '-',
            stat.st_mode & S_IROTH ? 'r' : '-',
            stat.st_mode & S_IWOTH ? 'w' : '-',
            stat.st_mode & S_IXOTH ? 'x' : '-'
        );
        printf(" ");

        struct passwd* passwd = getpwuid(stat.st_uid);
        if (!passwd) {
            perror("Something went wrong with getpwuid");
            exit(1);
        }
        printf("%-16s", passwd->pw_name);

        struct group* group = getgrgid(stat.st_gid);
        if (!group) {
            perror("Something went wrong with getpgrgid");
            exit(1);
        }
        printf("%-16s", group->gr_name);

        if ((stat.st_mode & S_IFMT) == S_IFREG) {
            printf("%-10ld", stat.st_size);
        } else {
            printf("          ");
        }


        char* mtime = ctime(&stat.st_mtime);
        if (!mtime) {
            perror("Something went wrong with ctime");
            exit(1);
        }
        mtime[strlen(mtime) - 1] = '\0'; // delete '\n'
        printf("%s ", mtime);

        printf("%s\n", basename(argv[i]));
    }
}