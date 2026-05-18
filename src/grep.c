#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_help(FILE *out) {
  fprintf(out,
          "grep - procura strings dentro de ficheiros\n"
          "Sintaxe: grep [OPCOES] STRING FICHEIROS...\n"
          "Opcoes:\n"
          "  -h        mostra esta ajuda e sai\n"
          "  -c        nao mostra linhas, mostra o numero de ocorrencias\n"
          "  -n        indica numero da linha no inicio\n"
          "  -i        ignora maiusculas/minusculas\n"
          "  -v        inverte (mostra linhas que NAO tenham a string)\n");
}

static int contains_substr_ci(const char *hay, const char *needle) {
  size_t nlen = strlen(needle);
  if (nlen == 0) {
    return 1;
  }
  size_t hlen = strlen(hay);
  if (nlen > hlen) {
    return 0;
  }
  for (size_t i = 0; i + nlen <= hlen; i++) {
    size_t j = 0;
    for (; j < nlen; j++) {
      unsigned char hc = (unsigned char)hay[i + j];
      unsigned char nc = (unsigned char)needle[j];
      if ((unsigned char)tolower(hc) != (unsigned char)tolower(nc)) {
        break;
      }
    }
    if (j == nlen) {
      return 1;
    }
  }
  return 0;
}

static size_t count_occurrences_cs(const char *hay, const char *needle) {
  size_t nlen = strlen(needle);
  if (nlen == 0) {
    return 0;
  }
  size_t count = 0;
  const char *p = hay;
  for (;;) {
    const char *m = strstr(p, needle);
    if (!m) {
      break;
    }
    count++;
    p = m + nlen;
  }
  return count;
}

static size_t count_occurrences_ci(const char *hay, const char *needle) {
  size_t nlen = strlen(needle);
  if (nlen == 0) {
    return 0;
  }
  size_t hlen = strlen(hay);
  if (nlen > hlen) {
    return 0;
  }
  size_t count = 0;
  size_t i = 0;
  while (i + nlen <= hlen) {
    size_t j = 0;
    for (; j < nlen; j++) {
      unsigned char hc = (unsigned char)hay[i + j];
      unsigned char nc = (unsigned char)needle[j];
      if ((unsigned char)tolower(hc) != (unsigned char)tolower(nc)) {
        break;
      }
    }
    if (j == nlen) {
      count++;
      i += nlen;
    } else {
      i++;
    }
  }
  return count;
}

static int file_has_match(const char *line, const char *pattern, int ignore_case) {
  if (ignore_case) {
    return contains_substr_ci(line, pattern);
  }
  return strstr(line, pattern) != NULL;
}

static int process_file(const char *file, const char *pattern, int count_only,
                        int show_line_no, int ignore_case, int invert,
                        int prefix_file, size_t *out_count) {
  FILE *fp = fopen(file, "r");
  if (!fp) {
    fprintf(stderr, "grep: %s: %s\n", file, strerror(errno));
    return 1;
  }

  char *line = NULL;
  size_t cap = 0;
  unsigned long line_no = 0;
  size_t total = 0;

  for (;;) {
    errno = 0;
    ssize_t r = getline(&line, &cap, fp);
    if (r < 0) {
      break;
    }
    line_no++;

    int has = file_has_match(line, pattern, ignore_case);
    int ok = invert ? !has : has;
    if (!ok) {
      continue;
    }

    if (count_only) {
      if (invert) {
        total++;
      } else {
        total += ignore_case ? count_occurrences_ci(line, pattern)
                             : count_occurrences_cs(line, pattern);
      }
      continue;
    }

    if (prefix_file) {
      printf("%s:", file);
    }
    if (show_line_no) {
      printf("%lu:", line_no);
    }
    fputs(line, stdout);
    if (line[0] != '\0') {
      size_t n = strlen(line);
      if (line[n - 1] != '\n') {
        fputc('\n', stdout);
      }
    } else {
      fputc('\n', stdout);
    }
  }

  free(line);
  fclose(fp);

  if (out_count) {
    *out_count = total;
  }
  return 0;
}

int main(int argc, char **argv) {
  if (arg_has_flag(argc, argv, "-h")) {
    print_help(stdout);
    return 0;
  }

  int count_only = 0;
  int show_line_no = 0;
  int ignore_case = 0;
  int invert = 0;

  int i = 1;
  for (; i < argc; i++) {
    if (argv[i][0] != '-') {
      break;
    }
    if (strcmp(argv[i], "-c") == 0) {
      count_only = 1;
      continue;
    }
    if (strcmp(argv[i], "-n") == 0) {
      show_line_no = 1;
      continue;
    }
    if (strcmp(argv[i], "-i") == 0) {
      ignore_case = 1;
      continue;
    }
    if (strcmp(argv[i], "-v") == 0) {
      invert = 1;
      continue;
    }
    fprintf(stderr, "grep: opcao invalida: %s\n", argv[i]);
    print_help(stderr);
    return 1;
  }

  if (i >= argc) {
    fprintf(stderr, "grep: STRING em falta\n");
    print_help(stderr);
    return 1;
  }
  const char *pattern = argv[i++];

  if (i >= argc) {
    fprintf(stderr, "grep: FICHEIROS em falta\n");
    print_help(stderr);
    return 1;
  }

  int files_count = argc - i;
  int prefix_file = (files_count > 1) ? 1 : 0;

  int any_error = 0;
  for (; i < argc; i++) {
    size_t c = 0;
    if (process_file(argv[i], pattern, count_only, show_line_no, ignore_case,
                     invert, prefix_file, &c) != 0) {
      any_error = 1;
      continue;
    }
    if (count_only) {
      if (prefix_file) {
        printf("%s:%zu\n", argv[i], c);
      } else {
        printf("%zu\n", c);
      }
    }
  }

  return any_error ? 1 : 0;
}
