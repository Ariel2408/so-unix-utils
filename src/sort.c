#include "common.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(FILE *out) {
  fprintf(out,
          "sort - ordena ficheiros de texto\n"
          "Sintaxe: sort [OPCOES] FICHEIRO...\n"
          "Opcoes:\n"
          "  -h        mostra esta ajuda e sai\n"
          "  -d        ordenacao decrescente\n");
}

static int cmp_asc(const void *a, const void *b) {
  const char *sa = *(const char *const *)a;
  const char *sb = *(const char *const *)b;
  return strcmp(sa, sb);
}

static int cmp_desc(const void *a, const void *b) { return -cmp_asc(a, b); }

static int sort_one(const char *path, int desc) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    fprintf(stderr, "sort: %s: %s\n", path, strerror(errno));
    return 1;
  }

  char **lines = NULL;
  size_t used = 0;
  size_t cap = 0;
  char *line = NULL;
  size_t line_cap = 0;

  for (;;) {
    errno = 0;
    ssize_t r = getline(&line, &line_cap, fp);
    if (r < 0) {
      break;
    }
    if (used == cap) {
      cap = cap ? cap * 2 : 128;
      lines = xrealloc(lines, cap * sizeof(*lines));
    }
    lines[used++] = xstrdup(line);
  }

  free(line);
  fclose(fp);

  if (used == 0) {
    free(lines);
    char *out_path = xmalloc(strlen(path) + 6);
    sprintf(out_path, "%s.sort", path);
    FILE *out = fopen(out_path, "w");
    if (!out) {
      fprintf(stderr, "sort: %s: %s\n", out_path, strerror(errno));
      free(out_path);
      return 1;
    }
    fclose(out);
    free(out_path);
    return 0;
  }

  qsort(lines, used, sizeof(*lines), desc ? cmp_desc : cmp_asc);

  char *out_path = xmalloc(strlen(path) + 6);
  sprintf(out_path, "%s.sort", path);
  FILE *out = fopen(out_path, "w");
  if (!out) {
    fprintf(stderr, "sort: %s: %s\n", out_path, strerror(errno));
    for (size_t i = 0; i < used; i++) {
      free(lines[i]);
    }
    free(lines);
    free(out_path);
    return 1;
  }

  for (size_t i = 0; i < used; i++) {
    fputs(lines[i], out);
    if (lines[i][0] != '\0') {
      size_t n = strlen(lines[i]);
      if (lines[i][n - 1] != '\n') {
        fputc('\n', out);
      }
    } else {
      fputc('\n', out);
    }
  }

  if (fclose(out) != 0) {
    fprintf(stderr, "sort: %s: %s\n", out_path, strerror(errno));
    for (size_t i = 0; i < used; i++) {
      free(lines[i]);
    }
    free(lines);
    free(out_path);
    return 1;
  }

  for (size_t i = 0; i < used; i++) {
    free(lines[i]);
  }
  free(lines);
  free(out_path);
  return 0;
}

int main(int argc, char **argv) {
  if (arg_has_flag(argc, argv, "-h")) {
    print_help(stdout);
    return 0;
  }

  int desc = 0;
  int first_file = 1;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      desc = 1;
      first_file = i + 1;
      continue;
    }
    first_file = i;
    break;
  }

  if (first_file >= argc) {
    fprintf(stderr, "sort: argumentos em falta\n");
    print_help(stderr);
    return 1;
  }

  int any_error = 0;
  for (int i = first_file; i < argc; i++) {
    if (sort_one(argv[i], desc) != 0) {
      any_error = 1;
    }
  }

  return any_error ? 1 : 0;
}
