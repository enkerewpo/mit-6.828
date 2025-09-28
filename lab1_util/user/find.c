#include "kernel/types.h"

#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "kernel/stat.h"

#include "user/user.h"
char buf[512];

void __find(int idx, // "./abc/{fd's dirname}/{filename}". idx will point to end
                     // of "{fd's dirname}/"
            int fd, char *filename) {
  struct dirent de;

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0) {
      break;
    }
    if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
      continue;
    }
    // printf("__find: (buf=%s, idx=%d) %s\n", buf, idx, de.name);
    // concat de.name to buf and open it
    strcpy(buf + idx, de.name);
    buf[idx + strlen(de.name)] = '/';
    buf[idx + strlen(de.name) + 1] = '\0';
    int fd2;
    if ((fd2 = open(buf, O_RDONLY)) < 0) {
      fprintf(2, "__find: cannot open %s\n", buf);
      close(fd2);
      exit(1);
    }
    struct stat st;
    if (fstat(fd2, &st) < 0) {
      fprintf(2, "__find: cannot stat %s\n", buf);
      close(fd2);
      exit(1);
    }
    if (st.type == T_DIR) {
      // printf("__find: %s is a directory\n", buf);
      __find(idx + strlen(de.name) + 1, fd2, filename);
    } else if (st.type == T_FILE) {
      // printf("__find: %s is a file\n", buf);
      if (strcmp(de.name, filename) == 0) {
        printf("%s\n", buf);
      }
    }
    close(fd2);
  }
}

void find(char *directory, char *filename) {
  // printf("find: %s %s\n", directory, filename);
  int fd;
  struct stat st;

  if ((fd = open(directory, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", directory);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", directory);
    close(fd);
    return;
  }

  // if the path opended is not a directory, return
  if (st.type != T_DIR) {
    fprintf(2, "find: %s is not a directory\n", directory);
    close(fd);
    return;
  }

  // if the path opended is a directory, recursively find the filename under the
  // dir

  // first we copy the name of the directory to buf
  strcpy(buf, directory);
  buf[strlen(directory)] = '/';
  buf[strlen(directory) + 1] = '\0';

  __find(strlen(directory) + 1, fd, filename);

  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(2, "Usage: find <directory> <filename>\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}