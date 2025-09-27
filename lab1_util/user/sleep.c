#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int seconds;
  if (argc != 2) {
    fprintf(2, "Usage: sleep <seconds>\n");
    exit(1);
  }
  seconds = atoi(argv[1]);
  sleep(seconds);
  exit(0);
}