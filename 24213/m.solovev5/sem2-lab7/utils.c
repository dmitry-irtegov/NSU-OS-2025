#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

void extend_path(char *dir, char *next, char *path) {
  size_t dir_len = strlen(dir);
  size_t len = dir_len + strlen(next) + 2;
  if (dir[dir_len - 1] == '/') {
    snprintf(path, len, "%s%s", dir, next);
  } else {
    snprintf(path, len, "%s/%s", dir, next);
  }
}

int create_dir(char *path) {
  struct stat stats;
  int code = stat(path, &stats);

  if (code == ENOENT || !S_ISDIR(stats.st_mode)) {
    mkdir(path, 0777);
  } else if (code != 0) {
    perror(path);
    return -1;
  }
  return 0;
}

int copy_file(char *soure, char *destination) {
  int source_fd = open(soure, O_RDONLY);
  if (source_fd == -1) {
    perror(soure);
    return -1;
  }
  int destination_fd = open(destination, O_WRONLY | O_CREAT | O_TRUNC);
  if (destination_fd == -1) {
    perror(destination);
    return -1;
  }

  char buf[BUFSIZ];
  ssize_t read_bytes = 0;

  while ((read_bytes = read(source_fd, buf, BUFSIZ)) > 0) {
    ssize_t write_total = 0;
    while (write_total < read_bytes) {
      ssize_t bytes =
          write(destination_fd, buf + write_total, read_bytes - write_total);
      if (bytes < 0) {
        perror(destination);
        close(source_fd);
        close(destination_fd);
        return -1;
      }

      write_total += bytes;
    }
  }

  int code = 0;
  if (read_bytes < 0) {
    perror(soure);
    code = -1;
  }

  close(source_fd);
  close(destination_fd);
  return code;
}
