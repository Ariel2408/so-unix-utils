#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void print_help(FILE *out) {
  fprintf(out,
          "rm - apaga ficheiros\n"
          "Sintaxe: rm [OPCOES] FICHEIRO...\n"
          "Opcoes:\n"
          "  -h        mostra esta ajuda e sai\n"
          "  -y        confirma cada ficheiro antes de apagar\n");
}

int main(int argc, char **argv) {
  if (arg_has_flag(argc, argv, "-h")) {
    print_help(stdout);
    return 0;
  }

  int confirm_each = 0;
  int first_file = 1;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-y") == 0) {
      confirm_each = 1;
      first_file = i + 1;
      continue;
    }
    first_file = i;
    break;
  }

  if (first_file >= argc) {
    fprintf(stderr, "rm: argumentos em falta\n");
    print_help(stderr);
    return 1;
  }

  int any_error = 0;
  for (int i = first_file; i < argc; i++) {
    struct stat st;
    if (lstat(argv[i], &st) != 0) {
      fprintf(stderr, "rm: %s: %s\n", argv[i], strerror(errno));
      any_error = 1;
      continue;
    }
    if (confirm_each) {
      char prompt[4096];
      snprintf(prompt, sizeof(prompt), "rm: apagar '%s'? [y/N] ", argv[i]);
      if (!prompt_yes_no(prompt)) {
        continue;
      }
    }
    if (unlink(argv[i]) != 0) {
      fprintf(stderr, "rm: %s: %s\n", argv[i], strerror(errno));
      any_error = 1;
    }
  }

  return any_error ? 1 : 0;
}
