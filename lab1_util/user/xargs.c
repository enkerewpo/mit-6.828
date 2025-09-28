#include "kernel/types.h"

#include "kernel/stat.h"
#include "user/user.h"

void xargs(int argc, char *argv[]) {
  char line[100];
  char *n_argv[100];

  for (int i = 0; i < 100; i++) {
    n_argv[i] = 0;
  }

  for (int i = 1; i < argc; i++) {
    n_argv[i - 1] = argv[i];
  }

  int base_argc = argc - 1;

  while (1) {
    // Read one line at a time
    int i = 0;
    char c;
    while (read(0, &c, 1) > 0 && c != '\n' && i < sizeof(line) - 1) {
      line[i++] = c;
    }
    line[i] = '\0';

    // If we didn't read anything, we're done
    if (i == 0 && read(0, &c, 1) <= 0) {
      break;
    }

    // Process the line
    char *p = line;
    while (*p != '\0') {
      while (*p == ' ' || *p == '\t') {
        p++;
      }
      if (*p == '\0')
        break;

      char *word_start = p;
      while (*p != ' ' && *p != '\t' && *p != '\0') {
        p++;
      }

      int word_len = p - word_start;
      char *word = malloc(word_len + 1);
      if (word == 0) {
        fprintf(2, "xargs: malloc failed\n");
        exit(1);
      }

      for (int j = 0; j < word_len; j++) {
        word[j] = word_start[j];
      }
      word[word_len] = '\0';

      n_argv[base_argc] = word;
      base_argc++;

      while (*p == ' ' || *p == '\t') {
        p++;
      }
    }

    n_argv[base_argc] = 0;

    int pid = fork();
    if (pid < 0) {
      fprintf(2, "xargs: fork failed\n");
      exit(1);
    } else if (pid == 0) {
      if (exec(argv[1], n_argv) < 0) {
        fprintf(2, "xargs: exec failed\n");
        exit(1);
      }
    } else {
      int status;
      wait(&status);
    }

    base_argc = argc - 1;
    for (int i = base_argc; i < 100; i++) {
      if (n_argv[i] != 0) {
        free(n_argv[i]);
        n_argv[i] = 0;
      }
    }
  }
}

int main(int argc, char *argv[]) {
  xargs(argc, argv);
  return 0;
}