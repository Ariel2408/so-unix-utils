#include "common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void print_help(FILE *out) {
  fprintf(out,
          "cp - copia ficheiros\n"
          "Sintaxe: cp [OPCOES] ORIGEM DESTINO\n"
          "Opcoes:\n"
          "  -h        mostra esta ajuda e sai\n"
          "  -i        modo interativo (pergunta antes de sobrescrever DESTINO)\n");
}

static int copy_file(const char *src, const char *dst) {
  int in = open(src, O_RDONLY);
  if (in < 0) {
    fprintf(stderr, "cp: %s: %s\n", src, strerror(errno));
    return 1;
  }

  struct stat st;
  if (fstat(in, &st) != 0) {
    fprintf(stderr, "cp: %s: %s\n", src, strerror(errno));
    close(in);
    return 1;
  }

  mode_t mode = st.st_mode & 0777;
  int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, mode);
  if (out < 0) {
    fprintf(stderr, "cp: %s: %s\n", dst, strerror(errno));
    close(in);
    return 1;
  }

  unsigned char buf[64 * 1024];
  for (;;) {
    ssize_t r = read(in, buf, sizeof(buf));
    if (r < 0) {
      if (errno == EINTR) {
        continue;
      }
      fprintf(stderr, "cp: %s: %s\n", src, strerror(errno));
      close(in);
      close(out);
      return 1;
    }
    if (r == 0) {
      break;
    }
    if (write_all(out, buf, (size_t)r) < 0) {
      fprintf(stderr, "cp: %s: %s\n", dst, strerror(errno));
      close(in);
      close(out);
      return 1;
    }
  }

  if (close(out) != 0) {
    fprintf(stderr, "cp: %s: %s\n", dst, strerror(errno));
    close(in);
    return 1;
  }
  close(in);
  return 0;
}

int main(int argc, char **argv) {
  if (arg_has_flag(argc, argv, "-h")) {
    print_help(stdout);
    return 0;
  }

  int interactive = 0;
  const char *src = NULL;
  const char *dst = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0) {
      interactive = 1;
      continue;
    }
    if (!src) {
      src = argv[i];
      continue;
    }
    if (!dst) {
      dst = argv[i];
      continue;
    }
    fprintf(stderr, "cp: argumentos a mais\n");
    print_help(stderr);
    return 1;
  }

  if (!src || !dst) {
    fprintf(stderr, "cp: argumentos em falta\n");
    print_help(stderr);
    return 1;
  }

  if (access(dst, F_OK) == 0 && interactive) {
    char prompt[4096];
    snprintf(prompt, sizeof(prompt), "cp: sobrescrever '%s'? [y/N] ", dst);
    if (!prompt_yes_no(prompt)) {
      return 0;
    }
  }

  return copy_file(src, dst);
}
