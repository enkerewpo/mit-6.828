#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"
#include "kernel/riscv.h"

char found_secret[8];

int
main(int argc, char *argv[])
{
  // your code here.  you should write the secret to fd 2 using write
  // (e.g., write(2, secret, 8)

  // we need to find "my very very very secret pw is:" in our memory
  // char *end = sbrk(PGSIZE*64); // the old end addr
  // char *search_ed = end + 64 * PGSIZE;
  // int flag = -1;
  // for (char* i = end; i < search_ed; i++) {
  //   char c1 = *i;
  //   char c2 = *(i+1);
  //   if (c1 == 'm' && c2 == 'y') {
  //     *(i+50) = 0;
  //     printf("found magic, offset=%d(%d), debug: %s\n", (int)(i-end)/PGSIZE, (int)((i-end)%PGSIZE), i);
  //     flag = 1;
  //     char* secret_st = i + 32;
  //     strcpy(found_secret, secret_st);
  //     printf("secret: %s\n", found_secret);
  //     // break;
  //   }
  // }
  // if (flag < 0) printf("not found\n");
  // NOT WORKING!

  char *end = sbrk(17*PGSIZE);
  end += 16 * PGSIZE;
  *(end+40) = 0; 
  strcpy(found_secret, end+32);
  write(2, found_secret, 8);
  exit(1);
}
