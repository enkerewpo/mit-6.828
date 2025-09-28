#include "kernel/types.h"
#include "user/user.h"

void primes(int parent_pipe_read_fd) __attribute__((noreturn));
void primes(int parent_pipe_read_fd) {
  int base;
  int bytes_read = read(parent_pipe_read_fd, &base, sizeof(base));
  if (bytes_read != sizeof(base)) {
    close(parent_pipe_read_fd);
    exit(0);
  }
  printf("prime %d\n", base);

  int p[2];
  pipe(p);

  int pid = fork();
  if (pid == 0) {
    // child process
    close(p[1]);
    close(parent_pipe_read_fd);
    primes(p[0]);
  } else {
    // parent process
    close(p[0]);

    int n;
    while ((bytes_read = read(parent_pipe_read_fd, &n, sizeof(n))) ==
           sizeof(n)) {
      if (n % base != 0) {
        write(p[1], &n, sizeof(n));
      }
    }

    close(p[1]);
    close(parent_pipe_read_fd);

    int ret;
    wait(&ret);
    exit(0);
  }
}

int main(int argc, char *argv[]) {
  int main_pipe[2];
  pipe(main_pipe);

  int pid = fork();
  if (pid == 0) {
    // child process - start the pipeline
    close(main_pipe[1]);
    primes(main_pipe[0]); // main process feeds 2-280 to the worker process
                          // through pipe
  } else {
    // parent process - feed numbers to pipeline
    close(main_pipe[0]);

    for (int i = 2; i <= 280; i++) {
      write(main_pipe[1], &i, sizeof(i));
    }
    close(main_pipe[1]);

    int ret;
    wait(&ret);
    exit(0);
  }
}