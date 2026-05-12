#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 80
#define MAX_LINES_NUMBER 100
#define SLEEP_FACTOR 100000

typedef struct string_t {
  char *value;
  size_t length;
} string_t;

void *sorter_routine(void *args) {
  string_t *line = args;

  usleep(line->length * SLEEP_FACTOR);
  printf("%zu: %s", line->length, line->value);

  return 0;
}

int main() {
  int n = 0;
  string_t lines[MAX_LINES_NUMBER];
  pthread_t threads[MAX_LINES_NUMBER];
  char buf[BUFFER_SIZE];
  int overflow = 0;

  while (n <= MAX_LINES_NUMBER && fgets(buf, sizeof(buf), stdin) != NULL) {
    if (!overflow) {
      lines[n].value = strdup(buf);
      if (lines[n].value == NULL) {
        perror("Cannot read line");
        exit(1);
      }
    } else {
      lines[n].value = realloc(lines[n].value, lines[n].length + sizeof(buf));
      if (lines[n].value == NULL) {
        perror("Cannot read line");
        exit(1);
      }
      strcat(lines[n].value, buf);
    }

    lines[n].length = strlen(lines[n].value);
    overflow = lines[n].value[lines[n].length - 1] != '\n';
    n += !overflow;
  }

  for (int i = 0; i < n; i++) {
    int code = pthread_create(&threads[i], NULL, sorter_routine, &lines[i]);
    if (code != 0) {
      char err_buf[256];
      strerror_r(code, err_buf, sizeof(err_buf));
      fprintf(stderr, "Cannot create thread: %s\n", err_buf);
      exit(1);
    }
  }

  for (int i = 0; i < n; i++) {
    int code = pthread_join(threads[i], NULL);
    if (code != 0) {
      char err_buf[256];
      strerror_r(code, err_buf, sizeof(err_buf));
      fprintf(stderr, "Cannot create thread: %s\n", err_buf);
      exit(1);
    }
  }

  for (int i = 0; i < n; i++) {
    free(lines[i].value);
  }

  exit(0);
}
