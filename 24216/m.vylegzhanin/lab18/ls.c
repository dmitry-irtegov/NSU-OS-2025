#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>

void print_file_info(const char *path) {
    struct stat file_stat;
    char buffer[512];
    int pos = 0;
    
    if (lstat(path, &file_stat) == -1) {
        perror(path);
        return;
    }
    
    if (S_ISDIR(file_stat.st_mode)) {
        buffer[pos++] = 'd';
    } else if (S_ISREG(file_stat.st_mode)) {
        buffer[pos++] = '-';
    } else {
        buffer[pos++] = '?';
    }
    
    buffer[pos++] = (file_stat.st_mode & S_IRUSR) ? 'r' : '-';
    buffer[pos++] = (file_stat.st_mode & S_IWUSR) ? 'w' : '-';
    buffer[pos++] = (file_stat.st_mode & S_IXUSR) ? 'x' : '-';
    buffer[pos++] = (file_stat.st_mode & S_IRGRP) ? 'r' : '-';
    buffer[pos++] = (file_stat.st_mode & S_IWGRP) ? 'w' : '-';
    buffer[pos++] = (file_stat.st_mode & S_IXGRP) ? 'x' : '-';
    buffer[pos++] = (file_stat.st_mode & S_IROTH) ? 'r' : '-';
    buffer[pos++] = (file_stat.st_mode & S_IWOTH) ? 'w' : '-';
    buffer[pos++] = (file_stat.st_mode & S_IXOTH) ? 'x' : '-';
    
    pos += sprintf(buffer + pos, " %3ld", (long)file_stat.st_nlink);
    
    struct passwd *pw = getpwuid(file_stat.st_uid);
    if (pw != NULL) {
        pos += sprintf(buffer + pos, " %-8s", pw->pw_name);
    } else {
        pos += sprintf(buffer + pos, " %-8d", file_stat.st_uid);
    }
    
    struct group *gr = getgrgid(file_stat.st_gid);
    if (gr != NULL) {
        pos += sprintf(buffer + pos, " %-8s", gr->gr_name);
    } else {
        pos += sprintf(buffer + pos, " %-8d", file_stat.st_gid);
    }
    
    if (S_ISREG(file_stat.st_mode)) {
        pos += sprintf(buffer + pos, " %8ld", (long)file_stat.st_size);
    } else {
        pos += sprintf(buffer + pos, " %8s", "");
    }
    
    char time_buf[80];
    struct tm *tm_info = localtime(&file_stat.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", tm_info);
    pos += sprintf(buffer + pos, " %s", time_buf);
    
    char *path_copy = strdup(path);
    char *filename = basename(path_copy);
    sprintf(buffer + pos, " %s\n", filename);
    free(path_copy);
    
    fputs(buffer, stdout);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> [file2] ...\n", argv[0]);
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        print_file_info(argv[i]);
    }
    
    return 0;
}
