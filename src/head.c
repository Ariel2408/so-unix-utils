#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(FILE *out) {
  fprintf(out,
          "head - lista as primeiras linhas de um ficheiro\n"
          "Sintaxe: head [OPCOES] [FICHEIRO]...\n"
          "Opcoes:\n"
          "  -h        mostra esta ajuda e sai\n"
          "  -NUM      lista as primeiras NUM linhas (por omissao 10)\n"
          "  -n        mostra o numero da linha\n"
          "  -E        indica fim de linha com $\n");
}

static void print_line(const char *line, unsigned long line_no, int show_no,
                       int show_eol) {
  if (show_no) {
    printf("%lu:", line_no);
  }

  size_t len = strlen(line);
  if (!show_eol) {
    fwrite(line, 1, len, stdout);
    if (len == 0 || line[len - 1] != '\n') {
      fputc('\n', stdout);
    }
    return;
  }

  if (len > 0 && line[len - 1] == '\n') {
    fwrite(line, 1, len - 1, stdout);
    fputs("$\n", stdout);
  } else {
    fwrite(line, 1, len, stdout);
    fputs("$\n", stdout);
  }
}

static int process_stream(FILE *fp, long max_lines, int show_no, int show_eol) {
  char *line = NULL;
  size_t cap = 0;
  unsigned long line_no = 0;
  long printed = 0;

  while (printed < max_lines) {
    errno = 0;
    ssize_t r = getline(&line, &cap, fp);
    if (r < 0) {
      break;
    }
    line_no++;
    printed++;
    print_line(line, line_no, show_no, show_eol);
  }

  free(line);
  return 0;
}

int main(int argc, char **argv) {
  if (arg_has_flag(argc, argv, "-h")) {
    print_help(stdout);
    return 0;
  }

  long max_lines = 10;
  int show_no = 0;
  int show_eol = 0;

  int any_error = 0;
  int files_start = 1;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-n") == 0) {
      show_no = 1;
      files_start = i + 1;
      continue;
    }
    if (strcmp(argv[i], "-E") == 0) {
      show_eol = 1;
      files_start = i + 1;
      continue;
    }
    long v = 0;
    if (parse_dash_number(argv[i], &v)) {
      max_lines = v;
      files_start = i + 1;
      continue;
    }
    files_start = i;
    break;
  }

  if (max_lines < 0) {
    fprintf(stderr, "head: numero invalido\n");
    return 1;
  }

  if (files_start >= argc) {
    process_stream(stdin, max_lines, show_no, show_eol);
    return 0;
  }

  for (int i = files_start; i < argc; i++) {
    FILE *fp = fopen(argv[i], "r");
    if (!fp) {
      fprintf(stderr, "head: %s: %s\n", argv[i], strerror(errno));
      any_error = 1;
      continue;
    }
    process_stream(fp, max_lines, show_no, show_eol);
    fclose(fp);
  }

  return any_error ? 1 : 0;
}
