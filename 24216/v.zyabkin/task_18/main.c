#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <libgen.h>


typedef struct FileInfoBlock {
    char file_type;
    char file_rights[10];

    unsigned long links;

    char owner[64];
    char group[64];

    long size;
    char mtime[32];

    char name[256];
} FileInfoBlock;


void print_info(const FileInfoBlock *info) {
    printf("%c%s %2lu %-8s %-8s ",
        info->file_type,
        info->file_rights,
        info->links,
        info->owner,
        info->group
    );

    if (info->file_type == '-')
        printf("%8ld ", info->size);
    else
        printf("%8s ", "");

    printf("%12s %s\n", info->mtime, info->name);
}


int fill_info(FileInfoBlock *info, const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) {
        perror(path);
        return -1;
    }


    /* ###################
       #  Defining type  #
       ###################
    */
    if (S_ISDIR(st.st_mode)) {
        info->file_type = 'd';
    } else if (S_ISREG(st.st_mode)) {
        info->file_type = '-';
    } else {
        info->file_type = '?';
    }


    /* #####################
       #  Defining rights  #
       #####################
    */
    info->file_rights[0] = (st.st_mode & S_IRUSR) ? 'r' : '-';
    info->file_rights[1] = (st.st_mode & S_IWUSR) ? 'w' : '-';
    info->file_rights[2] = (st.st_mode & S_IXUSR) ? 'x' : '-';
    info->file_rights[3] = (st.st_mode & S_IRGRP) ? 'r' : '-';
    info->file_rights[4] = (st.st_mode & S_IWGRP) ? 'w' : '-';
    info->file_rights[5] = (st.st_mode & S_IXGRP) ? 'x' : '-';
    info->file_rights[6] = (st.st_mode & S_IROTH) ? 'r' : '-';
    info->file_rights[7] = (st.st_mode & S_IWOTH) ? 'w' : '-';
    info->file_rights[8] = (st.st_mode & S_IXOTH) ? 'x' : '-';
    info->file_rights[9] = '\0';


    /* ####################
       #  Defining links  #
       ####################
    */
    info->links = st.st_nlink;


    /* ####################
       #  Defining owner  #
       ####################
    */
    struct passwd *pw = getpwuid(st.st_uid);
    strncpy(info->owner, pw ? pw->pw_name : "unknown", sizeof(info->owner));


    /* ####################
       #  Defining group  #
       ####################
    */
    struct group *gr = getgrgid(st.st_gid);
    strncpy(info->group, gr ? gr->gr_name : "unknown", sizeof(info->group));


    /* ###################
       #  Defining size  #
       ###################
    */
    info->size = S_ISREG(st.st_mode) ? st.st_size : 0;


    /* ###################
       #  Defining date  #
       ###################
    */
    struct tm *tm = localtime(&st.st_mtime);
    strftime(info->mtime, sizeof(info->mtime), "%b %d %H:%M", tm);


    /* ###################
       #  Defining name  #
       ###################
    */
    char tmp[256];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp)-1] = '\0';
    strncpy(info->name, basename(tmp), sizeof(info->name));

    return 0;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        FileInfoBlock info;
        if (fill_info(&info, ".") == 0) { 
            print_info(&info);
            return 0;
        }
    }

    for (int i = 1; i < argc; i++) {
        FileInfoBlock info;
        if (fill_info(&info, argv[i]) == 0) {
            print_info(&info);
        }
    }

    return 0;
}
