#include "kernel/fcntl.h"
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "kernel/fs.h"
#include "user/user.h"

void test0();
void test1();
void test2();
void test3();

#define SZ 4096
char buf[SZ];

int
main(int argc, char *argv[])
{
  test0();
  test1();
  test2();
  test3();
  exit(0);
}

void
createfile(char *file, int nblock)
{
  int fd;
  char buf[BSIZE];
  int i;
  
  fd = open(file, O_CREATE | O_RDWR);
  if(fd < 0){
    printf("createfile %s failed\n", file);
    exit(-1);
  }
  for(i = 0; i < nblock; i++) {
    if(write(fd, buf, sizeof(buf)) != sizeof(buf)) {
      printf("write %s failed\n", file);
      exit(-1);
    }
  }
  close(fd);
}

void
readfile(char *file, int nbytes, int inc)
{
  char buf[BSIZE];
  int fd;
  int i;

  if(inc > BSIZE) {
    printf("readfile: inc too large\n");
    exit(-1);
  }
  if ((fd = open(file, O_RDONLY)) < 0) {
    printf("readfile open %s failed\n", file);
    exit(-1);
  }
  for (i = 0; i < nbytes; i += inc) {
    if(read(fd, buf, inc) != inc) {
      printf("read %s failed for block %d (%d)\n", file, i, nbytes);
      exit(-1);
    }
  }
  close(fd);
}

int ntas(int print)
{
  int n;
  char *c;

  if (statistics(buf, SZ) <= 0) {
    fprintf(2, "ntas: no stats\n");
  }
  c = strchr(buf, '=');
  n = atoi(c+2);
  if(print)
    printf("%s", buf);
  return n;
}

// Test reading small files concurrently
void
test0()
{
  char file[2];
  char dir[2];
  enum { N = 10, NCHILD = 3 };
  int m, n;

  dir[0] = '0';
  dir[1] = '\0';
  file[0] = 'F';
  file[1] = '\0';

  printf("start test0\n");
  for(int i = 0; i < NCHILD; i++){
    dir[0] = '0' + i;
    mkdir(dir);
    if (chdir(dir) < 0) {
      printf("chdir failed\n");
      exit(1);
    }
    unlink(file);
    createfile(file, N);
    if (chdir("..") < 0) {
      printf("chdir failed\n");
      exit(1);
    }
  }
  m = ntas(0);
  for(int i = 0; i < NCHILD; i++){
    dir[0] = '0' + i;
    int pid = fork();
    if(pid < 0){
      printf("fork failed");
      exit(-1);
    }
    if(pid == 0){
      if (chdir(dir) < 0) {
        printf("chdir failed\n");
        exit(1);
      }

      readfile(file, N*BSIZE, 1);

      exit(0);
    }
  }
  
  int status = 0;
  for(int i = 0; i < NCHILD; i++){
    wait(&status);
    if (status != 0) {
      printf("FAIL: a child failed\n");
      exit(1);
    }  
  }
  
  printf("test0 results:\n");
  n = ntas(1);
  if (n-m < 500)
    printf("test0: OK\n");
  else
    printf("test0: FAIL\n");
}

// Test bcache evictions by reading a large file concurrently
void test1()
{
  char file[3];
  enum { N = 200, BIG=100, NCHILD=2 };
  
  printf("start test1\n");
  file[0] = 'B';
  file[2] = '\0';
  for(int i = 0; i < NCHILD; i++){
    file[1] = '0' + i;
    unlink(file);
    if (i == 0) {
      createfile(file, BIG);
    } else {
      createfile(file, 1);
    }
  }
  for(int i = 0; i < NCHILD; i++){
    file[1] = '0' + i;
    int pid = fork();
    if(pid < 0){
      printf("fork failed");
      exit(-1);
    }
    if(pid == 0){
      if (i==0) {
        for (i = 0; i < N; i++) {
          readfile(file, BIG*BSIZE, BSIZE);
        }
        unlink(file);
        exit(0);
      } else {
        for (i = 0; i < N*20; i++) {
          readfile(file, 1, BSIZE);
        }
        unlink(file);
      }
      exit(0);
    }
  }

  int status = 0;
  for(int i = 0; i < NCHILD; i++){
    wait(&status);
    if (status != 0) {
      printf("FAIL: a child failed\n");
      exit(1);
    }  
  }

  printf("\ntest1 OK\n");
}

//
// test concurrent creates.
//
void
test2()
{
  int nc = 4;
  char file[16];

  printf("start test2\n");
  
  mkdir("d2");

  file[0] = 'd';
  file[1] = '2';
  file[2] = '/';

  // remove any stale existing files.
  for(int i = 0; i < 50; i++){
    for(int ci = 0; ci < nc; ci++){
      file[3] = 'a' + ci;
      file[4] = '0' + i;
      file[5] = '\0';
      unlink(file);
    }
  }

  int pids[nc];
  for(int ci = 0; ci < nc; ci++){
    pids[ci] = fork();
    if(pids[ci] < 0){
      printf("test2: fork failed\n");
      exit(1);
    }
    if(pids[ci] == 0){
      char me = "abcdefghijklmnop"[ci];
      int pid = getpid();
      int nf = (ci == 0 ? 10 : 15);

      // create nf files.
      for(int i = 0; i < nf; i++){
        file[3] = me;
        file[4] = '0' + i;
        file[5] = '\0';
        int fd = open(file, O_CREATE | O_RDWR);
        if(fd < 0){
          printf("test2: create %s failed\n", file);
          exit(1);
        }
        int xx = (pid << 16) | i;
        for(int nw = 0; nw < 2; nw++){
          // the sleep() increases the chance of simultaneous
          // calls to bget().
          sleep(1);
          if(write(fd, &xx, sizeof(xx)) <= 0){
            printf("test2: write %s failed\n", file);
            exit(1);
          }
        }
        close(fd);
      }

      // read back the nf files.
      for(int i = 0; i < nf; i++){
        file[3] = me;
        file[4] = '0' + i;
        file[5] = '\0';
        // printf("r %s\n", file);
        int fd = open(file, O_RDWR);
        if(fd < 0){
          printf("test2: open %s failed\n", file);
          exit(1);
        }
        int xx = (pid << 16) | i;
        for(int nr = 0; nr < 2; nr++){
          int z = 0;
          sleep(1);
          int n = read(fd, &z, sizeof(z));
          if(n != sizeof(z)){
            printf("test2: read %s returned %d, expected %ld\n", file, n, sizeof(z));
            exit(1);
          }
          if(z != xx){
            printf("test2: file %s contained %d, not %d\n", file, z, xx);
            exit(1);
          }
        }
        close(fd);
      }

      // delete the nf files.
      for(int i = 0; i < nf; i++){
        file[3] = me;
        file[4] = '0' + i;
        file[5] = '\0';
        //printf("u %s\n", file);
        if(unlink(file) != 0){
          printf("test2: unlink %s failed\n", file);
          exit(1);
        }
      }

      exit(0);
    }
  }

  int ok = 1;

  for(int ci = 0; ci < nc; ci++){
    int st = 0;
    int ret = wait(&st);
    if(ret <= 0){
      printf("test2: wait() failed\n");
      ok = 0;
    }
    if(st != 0)
      ok = 0;
  }
  if(ok) {
    printf("\ntest2 OK\n");
  } else {
    printf("test2 failed\n");
  }
}

//
// generate big log transactions to check that bget() can
// make use of any of the NBUF buffers for any block number.
//
void
test3()
{
  int nc = 5;
  char file[16];

  printf("start test3\n");
  
  mkdir("d2");

  file[0] = 'd';
  file[1] = '2';
  file[2] = '/';

  int pids[nc];
  for(int ci = 0; ci < nc; ci++){
    pids[ci] = fork();
    if(pids[ci] < 0){
      printf("test3: fork failed\n");
      exit(1);
    }
    if(pids[ci] == 0){
      file[3] = 'a' + ci;
      file[4] = '\0';
      unlink(file);
      int fd = open(file, O_CREATE | O_RDWR);
      if(fd < 0){
        printf("test3: create %s failed\n", file);
        exit(1);
      }
      write(fd, "x", 1);
      static char junk[12*512];
      for(int i = 0; i < 12; i++){
        sleep(1);
        write(fd, junk, sizeof(junk));
      }
      exit(0);
    }
  }

  int ok = 1;

  for(int ci = 0; ci < nc; ci++){
    int st = 0;
    int ret = wait(&st);
    if(ret <= 0){
      printf("test3: wait() failed\n");
      ok = 0;
    }
    if(st != 0)
      ok = 0;
  }

  for(int ci = 0; ci < nc; ci++){
    file[3] = 'a' + ci;
    file[4] = '\0';
    unlink(file);
  }

  if(ok) {
    printf("\ntest3 OK\n");
  } else {
    printf("test3 failed\n");
  }
}
