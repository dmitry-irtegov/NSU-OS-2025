#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STEPS 200000000000LL

typedef struct range_t {
  long long start;
  long long end;
  double result;
} range_t;

void *calculate_partial(void *args) {
  range_t *range = args;
  double result = 0.0;

  for (long long i = range->start; i < range->end; i++) {
    result += 1.0 / (i * 4.0 + 1.0);
    result -= 1.0 / (i * 4.0 + 3.0);
  }
  range->result = result;

  return range;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "%s: expected 1 argument, %d is given\n", argv[0], argc - 1);
    return 1;
  }
  int threads_number = atoi(argv[1]);
  if (threads_number <= 0) {
    fprintf(stderr, "%s: number of threads must be positive\n", argv[0]);
    return 1;
  }

  long long iteration_range = STEPS / threads_number;
  if (STEPS % threads_number != 0) {
    iteration_range++;
  }

  pthread_t threads[threads_number];
  range_t ranges[threads_number];
  int started_threads = 0;
  for (int i = 0; i < threads_number; i++) {
    ranges[i].start = i * iteration_range;
    if (ranges[i].start >= STEPS) {
      break;
    }
    ranges[i].end = (i + 1) * iteration_range;
    if (ranges[i].end >= STEPS) {
      ranges[i].end = STEPS;
    }
    int code = pthread_create(&threads[i], NULL, calculate_partial, &ranges[i]);
    if (code != 0) {
      char err_buf[256];
      strerror_r(code, err_buf, sizeof(err_buf));
      fprintf(stderr, "%s: %s\n", argv[0], err_buf);
      return 1;
    }
    started_threads++;
  }

  double pi = 0;
  for (int i = 0; i < started_threads; i++) {
    int code = pthread_join(threads[i], NULL);
    if (code != 0) {
      char err_buf[256];
      strerror_r(code, err_buf, sizeof(err_buf));
      fprintf(stderr, "%s: %s\n", argv[0], err_buf);
      return 1;
    }
    pi += ranges[i].result;
  }

  pi = pi * 4.0;
  printf("pi done - %.15g \n", pi);

  return 0;
}
