#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 8192
#define MAX_RETRIES 10
#define RETRY_SLEEP 1

typedef struct {
  char *src;
  char *dst;
} job_t;

typedef struct {
  pthread_t *thIDs;
  size_t count;
  size_t cap;
} thread_vector_t;

void thread_vector_init(thread_vector_t *a) {
  a->thIDs = NULL;
  a->count = 0;
  a->cap = 0;
}

int thread_vector_add(thread_vector_t *a, pthread_t tid) {
  if (a->count >= a->cap) {
    size_t new_cap = (a->cap == 0) ? 16 : a->cap * 2;
    pthread_t *new_thIDs = realloc(a->thIDs, new_cap * sizeof(pthread_t));
    if (!new_thIDs) return -1;
    a->thIDs = new_thIDs;
    a->cap = new_cap;
  }
  a->thIDs[a->count++] = tid;
  return 0;
}

void thread_vector_join(thread_vector_t *a) {
  for (size_t i = 0; i < a->count; i++) pthread_join(a->thIDs[i], NULL);
  free(a->thIDs);
  a->thIDs = NULL;
  a->count = 0;
  a->cap = 0;
}

int open_retry(const char *path, int flags, mode_t mode) {
  int fd;
  int retries = 0;
  while ((fd = open(path, flags, mode)) == -1) {
    if ((errno == EMFILE || errno == ENFILE) && retries++ < MAX_RETRIES)
      sleep(RETRY_SLEEP);
    else
      return -1;
  }
  return fd;
}

DIR *opendir_retry(const char *path) {
  DIR *dir;
  int retries = 0;
  while ((dir = opendir(path)) == NULL) {
    if ((errno == EMFILE || errno == ENFILE) && retries++ < MAX_RETRIES)
      sleep(RETRY_SLEEP);
    else
      return NULL;
  }
  return dir;
}

void *file_worker(void *arg) {
  job_t *j = (job_t *)arg;
  int src_fd = -1, dst_fd = -1;
  char buf[BUFFER_SIZE];
  ssize_t n, written;
  struct stat st;
  int error = 0;

  // 1. get stats
  if (stat(j->src, &st) != 0) {
    fprintf(stderr, "stat(%s): %s\n", j->src, strerror(errno));
    error = 1;
  } else if (!S_ISREG(st.st_mode)) {
    error = 1;
  } else {
    // 2. open src file
    src_fd = open_retry(j->src, O_RDONLY, 0);
    if (src_fd == -1) {
      fprintf(stderr, "open(%s): %s\n", j->src, strerror(errno));
      error = 1;
    }
  }

  // 3. open dst file (or create)
  if (!error && src_fd != -1) {
    dst_fd =
        open_retry(j->dst, O_WRONLY | O_CREAT | O_TRUNC, st.st_mode & 0777);

    if (dst_fd == -1) {
      fprintf(stderr, "create(%s): %s\n", j->dst, strerror(errno));
      error = 1;
    }
  }

  // 4. write content src -> dst
  if (!error && src_fd != -1 && dst_fd != -1) {
    while ((n = read(src_fd, buf, sizeof(buf))) > 0) {
      char *p = buf;
      ssize_t left = n;
      while (left > 0) {
        written = write(dst_fd, p, left);
        if (written <= 0) {
          if (written == -1 && errno == EINTR) continue;
          fprintf(stderr, "write(%s): %s\n", j->dst, strerror(errno));
          error = 1;
          break;
        }
        left -= written;
        p += written;
      }
      if (error) break;
    }
    if (n == -1 && !error) {
      fprintf(stderr, "read(%s): %s\n", j->src, strerror(errno));
    }
  }
  // 5. close fds
  if (src_fd != -1) close(src_fd);
  if (dst_fd != -1) close(dst_fd);
  free(j->src);
  free(j->dst);
  free(j);
  return NULL;
}

void *dir_worker(void *arg) {
  job_t *j = (job_t *)arg;
  DIR *dir = NULL;
  struct dirent *entry = NULL, *result = NULL;
  long name_max;
  size_t buf_sz;
  char *buf = NULL;
  struct stat st, src_st;
  thread_vector_t children;
  int mkdir_failed = 0;
  mode_t dir_mode = 0755;  // fallback

  // 1. get stats (and access permitions for new dir)
  if (stat(j->src, &src_st) == 0 && S_ISDIR(src_st.st_mode)) {
    dir_mode = src_st.st_mode & 0777;
  } else {
    fprintf(stderr, "stat(%s) failed: %s\n", j->src, strerror(errno));
  }

  // 2. create new dir
  if (mkdir(j->dst, dir_mode) == -1 && errno != EEXIST) {
    fprintf(stderr, "mkdir(%s): %s\n", j->dst, strerror(errno));
    mkdir_failed = 1;
  }

  // 3. open new
  if (!mkdir_failed) {
    dir = opendir_retry(j->src);
    if (dir == NULL) {
      fprintf(stderr, "opendir(%s): %s\n", j->src, strerror(errno));
    }
  }

  // 4. buf for readdir
  if (dir != NULL) {
    name_max = pathconf(j->src, _PC_NAME_MAX);
    if (name_max == -1) name_max = 255;
    buf_sz = sizeof(struct dirent) + name_max + 1;
    buf = malloc(buf_sz);
    if (buf == NULL) {
      fprintf(stderr, "malloc dirent buffer: %s\n", strerror(errno));
    } else {
      entry = (struct dirent *)buf;
      thread_vector_init(&children);
    }
  }

  // 5.work :)
  if (entry != NULL) {
    while (1) {
      errno = 0;
      if (readdir_r(dir, entry, &result) != 0) {
        fprintf(stderr, "readdir_r(%s): %s\n", j->src, strerror(errno));
        break;
      }
      if (result == NULL) break;

      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        continue;

      // 2 bytes for / before filename and \0
      char *child_src = malloc(strlen(j->src) + strlen(entry->d_name) + 2);
      char *child_dst = malloc(strlen(j->dst) + strlen(entry->d_name) + 2);
      if (!child_src || !child_dst) {
        free(child_src);
        free(child_dst);
        continue;
      }
      sprintf(child_src, "%s/%s", j->src, entry->d_name);
      sprintf(child_dst, "%s/%s", j->dst, entry->d_name);

      if (lstat(child_src, &st) != 0) {
        fprintf(stderr, "lstat(%s): %s\n", child_src, strerror(errno));
        free(child_src);
        free(child_dst);
        continue;
      }

      job_t *child_job = malloc(sizeof(job_t));
      if (!child_job) {
        free(child_src);
        free(child_dst);
        continue;
      }
      child_job->src = child_src;
      child_job->dst = child_dst;

      pthread_t tid;
      int rc = -1;
      // new thread for dir or file
      if (S_ISDIR(st.st_mode))
        rc = pthread_create(&tid, NULL, dir_worker, child_job);
      else if (S_ISREG(st.st_mode))
        rc = pthread_create(&tid, NULL, file_worker, child_job);
      else {
        /* ignore other types */
        free(child_src);
        free(child_dst);
        free(child_job);
        continue;
      }

      if (rc == 0) {
        thread_vector_add(&children, tid);
      } else {
        fprintf(stderr, "pthread_create: %s\n", strerror(rc));
        free(child_src);
        free(child_dst);
        free(child_job);
      }
    }
  }

  if (entry != NULL) thread_vector_join(&children);

  if (dir != NULL) closedir(dir);
  free(buf);
  free(j->src);
  free(j->dst);
  free(j);
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <src_dir> <dst_dir>\n", argv[0]);
    return 1;
  }

  job_t *root = malloc(sizeof(job_t));
  if (!root) {
    perror("malloc");
    return 1;
  }
  root->src = strdup(argv[1]);
  root->dst = strdup(argv[2]);
  if (!root->src || !root->dst) {
    perror("strdup");
    free(root->src);
    free(root->dst);
    free(root);
    return 1;
  }

  pthread_t root_tid;
  if (pthread_create(&root_tid, NULL, dir_worker, root) != 0) {
    fprintf(stderr, "pthread_create(root): %s\n", strerror(errno));
    free(root->src);
    free(root->dst);
    free(root);
    return 1;
  }
  pthread_join(root_tid, NULL);
  return 0;
}