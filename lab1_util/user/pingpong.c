#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int p1[2], p2[2]; // two pipes: one for each direction
  int pid;
  char buf;

  pipe(p1); // parent -> child
  pipe(p2); // child -> parent

  pid = fork();

  if (pid == 0) {
    // Child process
    close(p1[1]); // close write end of parent->child pipe
    close(p2[0]); // close read end of child->parent pipe

    read(p1[0], &buf, 1);
    printf("%d: received ping\n", getpid());

    write(p2[1], &buf, 1);

    close(p1[0]);
    close(p2[1]);
    exit(0);
  } else {
    // Parent process
    close(p1[0]); // close read end of parent->child pipe
    close(p2[1]); // close write end of child->parent pipe

    buf = 'x'; // any byte will do
    write(p1[1], &buf, 1);

    read(p2[0], &buf, 1);
    printf("%d: received pong\n", getpid());

    close(p1[1]);
    close(p2[0]);
    exit(0);
  }
}