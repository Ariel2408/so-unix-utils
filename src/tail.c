#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *s;
  unsigned long line_no;
} TailLine;

static void print_help(FILE *out) {
  fprintf(out,
          "tail - lista as ultimas linhas de um ficheiro\n"
          "Sintaxe: tail [OPCOES] [FICHEIRO]...\n"
          "Opcoes:\n"
          "  -h        mostra esta ajuda e sai\n"
          "  -NUM      lista as ultimas NUM linhas (por omissao 10)\n"
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

static int process_stream(FILE *fp, long last_n, int show_no, int show_eol) {
  if (last_n <= 0) {
    return 0;
  }

  size_t cap = (size_t)last_n;
  TailLine *buf = xmalloc(cap * sizeof(*buf));
  for (size_t i = 0; i < cap; i++) {
    buf[i].s = NULL;
    buf[i].line_no = 0;
  }

  char *line = NULL;
  size_t line_cap = 0;
  unsigned long line_no = 0;

  size_t used = 0;
  size_t pos = 0;

  for (;;) {
    errno = 0;
    ssize_t r = getline(&line, &line_cap, fp);
    if (r < 0) {
      break;
    }
    line_no++;
    char *copy = xstrdup(line);

    if (used < cap) {
      buf[used].s = copy;
      buf[used].line_no = line_no;
      used++;
    } else {
      free(buf[pos].s);
      buf[pos].s = copy;
      buf[pos].line_no = line_no;
      pos = (pos + 1) % cap;
    }
  }

  if (used == 0) {
    free(line);
    free(buf);
    return 0;
  }

  size_t start = 0;
  size_t count_out = used;
  if (used == cap) {
    start = pos;
    count_out = cap;
  }

  for (size_t i = 0; i < count_out; i++) {
    size_t idx = (start + i) % cap;
    print_line(buf[idx].s, buf[idx].line_no, show_no, show_eol);
  }

  for (size_t i = 0; i < used; i++) {
    free(buf[i].s);
  }
  free(buf);
  free(line);
  return 0;
}

int main(int argc, char **argv) {
  if (arg_has_flag(argc, argv, "-h")) {
    print_help(stdout);
    return 0;
  }

  long last_n = 10;
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
      last_n = v;
      files_start = i + 1;
      continue;
    }
    files_start = i;
    break;
  }

  if (last_n < 0) {
    fprintf(stderr, "tail: numero invalido\n");
    return 1;
  }

  if (files_start >= argc) {
    process_stream(stdin, last_n, show_no, show_eol);
    return 0;
  }

  for (int i = files_start; i < argc; i++) {
    FILE *fp = fopen(argv[i], "r");
    if (!fp) {
      fprintf(stderr, "tail: %s: %s\n", argv[i], strerror(errno));
      any_error = 1;
      continue;
    }
    process_stream(fp, last_n, show_no, show_eol);
    fclose(fp);
  }

  return any_error ? 1 : 0;
}
