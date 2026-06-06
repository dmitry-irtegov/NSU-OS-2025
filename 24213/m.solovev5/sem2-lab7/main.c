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
  char *src_path = extend_path(frame->src, frame->name);
  char *dest_path = extend_path(frame->dest, frame->name);
  struct stat stats;
  int code = stat(src_path, &stats);
  if (code != 0) {
    perror(src_path);
    free(src_path);
    free(dest_path);
    return args;
  }

  int src_fd = open(src_path, O_RDONLY);
  while (src_fd == -1 && errno == EMFILE) {
    src_fd = open(src_path, O_RDONLY);
  }
  if (src_fd == -1) {
    perror(src_path);
    return args;
  }
  int dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, stats.st_mode);
  while (dest_fd == -1 && errno == EMFILE) {
    dest_fd = open(dest_path, O_WRONLY | O_CREAT | O_TRUNC, stats.st_mode);
  }
  if (dest_fd == -1) {
    perror(dest_path);
    return args;
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
        return args;
      }

      write_total += bytes;
    }
  }

  if (read_bytes < 0) {
    perror(src_path);
  }

  close(src_fd);
  close(dest_fd);
  free(src_path);
  free(dest_path);
  return args;
}

void *handle_dir(void *args) {
  frame_t *frame = args;
  char *dest_path = extend_path(frame->dest, frame->name);
  if (create_dir(dest_path) != 0) {
    free(dest_path);
    return NULL;
  }
  char *src_path = extend_path(frame->src, frame->name);
  DIR *src_dir = opendir(src_path);
  while (src_dir == NULL && errno == EMFILE) {
    src_dir = opendir(src_path);
  }
  if (src_dir == NULL) {
    perror(src_path);
    free(dest_path);
    free(src_path);
    return args;
  }

  size_t threads_cnt = 0;
  size_t threads_cap = 16;
  pthread_t *threads = malloc(sizeof(pthread_t) * threads_cap);

  size_t size = sizeof(struct dirent) + pathconf(src_path, _PC_NAME_MAX) + 1;
  struct dirent *entry = malloc(size);
  struct dirent *result;
  int code = 0;
  while ((code = readdir_r(src_dir, entry, &result)) == 0 && result != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    char *filename = extend_path(src_path, entry->d_name);
    struct stat stats;
    code = stat(filename, &stats);
    if (code != 0) {
      perror(filename);
      free(filename);
      continue;
    }
    if (threads_cnt >= threads_cap) {
      threads_cap *= 2;
      threads = realloc(threads, sizeof(pthread_t) * threads_cap);
    }
    frame_t *nested = malloc(sizeof(frame_t));
    nested->src = src_path;
    nested->dest = dest_path;
    nested->name = strdup(entry->d_name);
    if (S_ISDIR(stats.st_mode)) {
      code = pthread_create(&threads[threads_cnt], NULL, handle_dir, nested);
      if (code != 0) {
        char err_buf[256];
        strerror_r(code, err_buf, sizeof(err_buf));
        fprintf(stderr, "%s: %s\n", filename, err_buf);
        free(nested->name);
        free(nested);
        continue;
      }
      threads_cnt++;
    } else if (S_ISREG(stats.st_mode)) {
      code = pthread_create(&threads[threads_cnt], NULL, handle_file, nested);
      if (code != 0) {
        char err_buf[256];
        strerror_r(code, err_buf, sizeof(err_buf));
        fprintf(stderr, "%s: %s\n", filename, err_buf);
        free(nested->name);
        free(nested);
        continue;
      }
      threads_cnt++;
    }
  }
  free(entry);

  void *thread_result;
  for (int i = 0; i < threads_cnt; i++) {
    pthread_join(threads[i], &thread_result);
    frame_t *nested = thread_result;
    free(nested->name);
    free(nested);
  }

  free(dest_path);
  free(src_path);
  free(threads);
  return args;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Expected 2 arguments, %d is given", argc);
    return 1;
  }
  frame_t frame = {argv[1], argv[2], ""};
  char *src = realpath(argv[1], NULL);
  char *dest = realpath(argv[2], NULL);
  for (int i = 0; src[i] == dest[i] || src[i] == '\0'; i++) {
    if (src[i] == '\0') {
      fprintf(stderr, "Cannot copy to child directory\n");
      free(src);
      free(dest);
      return 1;
    }
  }
  free(src);
  free(dest);

  void *result = handle_dir(&frame);
  if (result == NULL) {
    return 1;
  }

  return 0;
}
