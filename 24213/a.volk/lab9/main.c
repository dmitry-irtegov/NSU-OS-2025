#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
  pid_t pid = fork();

  if (pid == -1) {
    perror("fork failed");
    exit(1);
  }

  if (pid != 0) {
    // parent
#ifndef NOWAIT
    int status;
    pid_t cpid = wait(&status);
    assert(WIFEXITED(status) && cpid == pid);
#endif

  } else {
    // child
    static char executable[] = "cat";
    static char *const args[] = {executable, "./big_file.txt", NULL};
    execvp(executable, args);

    perror("execvp failed");
    abort();
  }

  return 0;
}
