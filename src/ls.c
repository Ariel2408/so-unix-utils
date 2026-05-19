#include "common.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/*
 * ls.c
 * Implementação simples de "ls" (listar diretórios).
 * Suporta:
 *   -h   ajuda
 *   -l   listagem longa: nome, tamanho, data de modificação
 *   -ox  ordena por: n (nome), s (tamanho), d (data)
 *   -c   listagem em colunas (assume terminal de 80 colunas)
 *
 * Nota: esta versão ignora "." e ".." e usa lstat() para obter metadata.
 */

typedef struct {
  char *name;
  struct stat st;
} Entry;

static void print_help(FILE *out) {
  fprintf(out,
          "ls - lista ficheiros num diretorio\n"
          "Sintaxe: ls [OPCOES] [CAMINHO]\n"
          "Opcoes:\n"
          "  -h        mostra esta ajuda e sai\n"
          "  -l        listagem longa (nome, dimensao, data)\n"
          "  -ox       ordena por: n (nome), s (dimensao), d (data)\n"
          "  -c        listagem por colunas\n");
}

/* Comparador por nome (ordem lexicográfica). */
static int cmp_name(const void *a, const void *b) {
  const Entry *ea = (const Entry *)a;
  const Entry *eb = (const Entry *)b;
  return strcmp(ea->name, eb->name);
}

/* Comparador por tamanho; em empate usa nome para desempate estável. */
static int cmp_size(const void *a, const void *b) {
  const Entry *ea = (const Entry *)a;
  const Entry *eb = (const Entry *)b;
  if (ea->st.st_size < eb->st.st_size) {
    return -1;
  }
  if (ea->st.st_size > eb->st.st_size) {
    return 1;
  }
  return strcmp(ea->name, eb->name);
}

/* Comparador por data de modificação (mtime); em empate usa nome. */
static int cmp_date(const void *a, const void *b) {
  const Entry *ea = (const Entry *)a;
  const Entry *eb = (const Entry *)b;
  if (ea->st.st_mtime < eb->st.st_mtime) {
    return -1;
  }
  if (ea->st.st_mtime > eb->st.st_mtime) {
    return 1;
  }
  return strcmp(ea->name, eb->name);
}

/* Formato "longo": nome, tamanho e data/hora (com base em st_mtime). */
static void print_long(const Entry *e) {
  char buf[64];
  struct tm tmv;
  localtime_r(&e->st.st_mtime, &tmv);
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tmv);
  printf("%s\t%lld\t%s\n", e->name, (long long)e->st.st_size, buf);
}

/*
 * Imprime nomes em colunas.
 * Calcula a largura da maior string e distribui por um terminal assumido de 80.
 */
static void print_columns(Entry *entries, size_t n) {
  if (n == 0) {
    return;
  }
  size_t maxlen = 0;
  for (size_t i = 0; i < n; i++) {
    size_t len = strlen(entries[i].name);
    if (len > maxlen) {
      maxlen = len;
    }
  }
  size_t width = maxlen + 2;
  size_t term = 80;
  size_t cols = width ? (term / width) : 1;
  if (cols == 0) {
    cols = 1;
  }
  size_t rows = (n + cols - 1) / cols;

  for (size_t r = 0; r < rows; r++) {
    for (size_t c = 0; c < cols; c++) {
      size_t idx = r * cols + c;
      if (idx >= n) {
        break;
      }
      const char *name = entries[idx].name;
      fputs(name, stdout);
      size_t pad = width - strlen(name);
      if (c + 1 < cols && idx + 1 < n) {
        for (size_t p = 0; p < pad; p++) {
          fputc(' ', stdout);
        }
      }
    }
    fputc('\n', stdout);
  }
}

/* Caso "path" não seja diretório, trata como um ficheiro único. */
static int list_single(const char *path, int long_list) {
  Entry e;
  e.name = (char *)path;
  if (lstat(path, &e.st) != 0) {
    fprintf(stderr, "ls: %s: %s\n", path, strerror(errno));
    return 1;
  }
  if (long_list) {
    print_long(&e);
  } else {
    puts(path);
  }
  return 0;
}

/*
 * Lista um diretório, recolhendo entradas numa lista dinâmica para poder ordenar.
 * "order" decide o comparador; "long_list" e "columns" controlam o output.
 */
static int list_dir(const char *path, int long_list, int columns, char order) {
  DIR *dir = opendir(path);
  if (!dir) {
    if (errno == ENOTDIR) {
      return list_single(path, long_list);
    }
    fprintf(stderr, "ls: %s: %s\n", path, strerror(errno));
    return 1;
  }

  Entry *entries = NULL;
  size_t used = 0;
  size_t cap = 0;

  for (;;) {
    errno = 0;
    struct dirent *de = readdir(dir);
    if (!de) {
      break;
    }
    if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) {
      continue;
    }

    /* Faz lstat ao caminho completo, mas guarda só o nome base para imprimir. */
    char *full = path_join2(path, de->d_name);
    struct stat st;
    if (lstat(full, &st) != 0) {
      free(full);
      continue;
    }
    free(full);

    if (used == cap) {
      cap = cap ? cap * 2 : 128;
      entries = xrealloc(entries, cap * sizeof(*entries));
    }
    entries[used].name = xstrdup(de->d_name);
    entries[used].st = st;
    used++;
  }

  closedir(dir);

  if (order == 's') {
    qsort(entries, used, sizeof(*entries), cmp_size);
  } else if (order == 'd') {
    qsort(entries, used, sizeof(*entries), cmp_date);
  } else {
    qsort(entries, used, sizeof(*entries), cmp_name);
  }

  if (long_list) {
    for (size_t i = 0; i < used; i++) {
      print_long(&entries[i]);
    }
  } else if (columns) {
    print_columns(entries, used);
  } else {
    for (size_t i = 0; i < used; i++) {
      puts(entries[i].name);
    }
  }

  for (size_t i = 0; i < used; i++) {
    free(entries[i].name);
  }
  free(entries);
  return 0;
}

int main(int argc, char **argv) {
  if (arg_has_flag(argc, argv, "-h")) {
    print_help(stdout);
    return 0;
  }

  int long_list = 0;
  int columns = 0;
  char order = 'n';
  const char *path = ".";

  /* Parsing simples de flags e (opcionalmente) um único caminho. */
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-l") == 0) {
      long_list = 1;
      continue;
    }
    if (strcmp(argv[i], "-c") == 0) {
      columns = 1;
      continue;
    }
    if (strncmp(argv[i], "-o", 2) == 0) {
      if (strlen(argv[i]) == 3) {
        char x = argv[i][2];
        if (x == 'n' || x == 's' || x == 'd') {
          order = x;
          continue;
        }
      }
      fprintf(stderr, "ls: opcao -o invalida\n");
      print_help(stderr);
      return 1;
    }
    if (strcmp(path, ".") != 0) {
      fprintf(stderr, "ls: demasiados argumentos\n");
      print_help(stderr);
      return 1;
    }
    path = argv[i];
  }

  if (long_list) {
    columns = 0;
  }

  return list_dir(path, long_list, columns, order);
}
