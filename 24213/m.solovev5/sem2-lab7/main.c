#include "utils.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct frame_t {
  char *src;
  char *dest;
  char *name;
} frame_t;

void *handle_file(void *args) {
  frame_t *frame = args;
  char src_path[strlen(frame->src) + strlen(frame->name) + 2];
  char dest_path[strlen(frame->dest) + strlen(frame->name) + 2];
  extend_path(frame->src, frame->name, src_path);
  extend_path(frame->dest, frame->name, dest_path);

  free(frame->name);
  free(frame->src);
  free(frame->dest);
  free(frame);

  struct stat stats;
  int code = stat(src_path, &stats);
  if (code != 0) {
    perror(src_path);
    return NULL;
  }

  int src_fd = open(src_path, O_RDONLY);
  while (src_fd == -1 && errno == EMFILE) {
    sleep(1);
    src_fd = open(src_path, O_RDONLY);
  }
  if (src_fd == -1) {
    perror(src_path);
    return NULL;
  }
  int dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, stats.st_mode);
  while (dest_fd == -1 && errno == EMFILE) {
    sleep(1);
    dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, stats.st_mode);
  }
  if (dest_fd == -1) {
    perror(dest_path);
    return NULL;
  }

  char buf[BUFSIZ];
  ssize_t read_bytes = 0;

  while ((read_bytes = read(src_fd, buf, BUFSIZ)) > 0) {
    ssize_t write_total = 0;
    while (write_total < read_bytes) {
      ssize_t bytes =
          write(dest_fd, buf + write_total, read_bytes - write_total);
      if (bytes < 0) {
        perror(dest_path);
        close(src_fd);
        close(dest_fd);
        return NULL;
      }

      write_total += bytes;
    }
  }

  if (read_bytes < 0) {
    perror(src_path);
  }

  close(src_fd);
  close(dest_fd);
  return NULL;
}

void *handle_dir(void *args) {
  frame_t *frame = args;
  char src_path[strlen(frame->src) + strlen(frame->name) + 2];
  char dest_path[strlen(frame->dest) + strlen(frame->name) + 2];
  extend_path(frame->src, frame->name, src_path);
  extend_path(frame->dest, frame->name, dest_path);

  free(frame->name);
  free(frame->src);
  free(frame->dest);
  free(frame);

  if (create_dir(dest_path) != 0) {
    return NULL;
  }

  DIR *src_dir = opendir(src_path);
  while (src_dir == NULL && errno == EMFILE) {
    sleep(1);
    src_dir = opendir(src_path);
  }
  if (src_dir == NULL) {
    perror(src_path);
    return args;
  }

  size_t size = sizeof(struct dirent) + pathconf(src_path, _PC_NAME_MAX) + 1;
  struct dirent *entry = malloc(size);
  struct dirent *result;
  int code = 0;
  while ((code = readdir_r(src_dir, entry, &result)) == 0 && result != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    char filename[strlen(src_path) + strlen(entry->d_name) + 2];
    extend_path(src_path, entry->d_name, filename);
    struct stat stats;
    code = stat(filename, &stats);
    if (code != 0) {
      perror(filename);
      continue;
    }
    pthread_t thread;
    frame_t *nested = malloc(sizeof(frame_t));
    nested->src = strdup(src_path);
    nested->dest = strdup(dest_path);
    nested->name = strdup(entry->d_name);
    if (S_ISDIR(stats.st_mode)) {
      while (1) {
        code = pthread_create(&thread, NULL, handle_dir, nested);
        if (code != EAGAIN) {
          break;
        }
        sleep(1);
      }
      if (code != 0) {
        char err_buf[256];
        strerror_r(code, err_buf, sizeof(err_buf));
        fprintf(stderr, "%s: %s\n", filename, err_buf);
        free(nested->name);
        free(nested->src);
        free(nested->dest);
        free(nested);
        continue;
      }
      pthread_detach(thread);
    } else if (S_ISREG(stats.st_mode)) {
      while (1) {
        code = pthread_create(&thread, NULL, handle_file, nested);
        if (code != EAGAIN) {
          break;
        }
        sleep(1);
      }
      if (code != 0) {
        char err_buf[256];
        strerror_r(code, err_buf, sizeof(err_buf));
        fprintf(stderr, "%s: %s\n", filename, err_buf);
        free(nested->name);
        free(nested->src);
        free(nested->dest);
        free(nested);
        continue;
      }
      pthread_detach(thread);
    }
  }

  free(entry);
  return args;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Expected 2 arguments, %d is given", argc);
    return 1;
  }

  char *src = realpath(argv[1], NULL);
  create_dir(argv[2]);
  char *dest = realpath(argv[2], NULL);
  for (int i = 0; src[i] == dest[i] || src[i] == '\0'; i++) {
    if (src[i] == '\0') {
      if (dest[i] == '/' || dest[i] == '\0') {
        fprintf(stderr, "Cannot copy to child directory\n");
        free(src);
        free(dest);
        return 1;
      }
      break;
    }
  }
  free(src);
  free(dest);

  frame_t *frame = malloc(sizeof(frame_t));
  frame->name = strdup("");
  frame->src = strdup(argv[1]);
  frame->dest = strdup(argv[2]);

  handle_dir(frame);
  pthread_exit(0);
}
