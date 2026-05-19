#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * common.c
 * Implementação dos helpers usados pelos vários comandos (cp, mv, ls, ...).
 * A ideia é centralizar código repetido: parsing, alocação e I/O robusto.
 */

/* Verifica se a flag existe em argv (comparação exata com strcmp). */
int arg_has_flag(int argc, char **argv, const char *flag) {
  for (int i = 1; i < argc; i++) {
    if (argv[i] && strcmp(argv[i], flag) == 0) {
      return 1;
    }
  }
  return 0;
}

/*
 * Interpreta strings do tipo "-10" (com sinal '-' e pelo menos 1 dígito).
 * Devolve 1 se conseguiu e o número for >= 0; caso contrário devolve 0.
 */
int parse_dash_number(const char *arg, long *out) {
  if (!arg || arg[0] != '-' || arg[1] == '\0') {
    return 0;
  }
  if (!isdigit((unsigned char)arg[1])) {
    return 0;
  }
  int ok = 0;
  long v = parse_long_strict(arg + 1, &ok);
  if (!ok || v < 0) {
    return 0;
  }
  if (out) {
    *out = v;
  }
  return 1;
}

/*
 * Faz parsing estrito de um long em base 10.
 * "Estrito" aqui significa: a string tem de ser toda consumida por strtol.
 */
long parse_long_strict(const char *s, int *ok) {
  if (ok) {
    *ok = 0;
  }
  if (!s || *s == '\0') {
    return 0;
  }
  errno = 0;
  char *end = NULL;
  long v = strtol(s, &end, 10);
  if (errno != 0) {
    return 0;
  }
  if (!end || *end != '\0') {
    return 0;
  }
  if (ok) {
    *ok = 1;
  }
  return v;
}

/*
 * Mostra um prompt (em stderr) e lê uma resposta do stdin.
 * O primeiro caractere não-branco decide: 'y'/'Y' -> sim; outros -> não.
 */
int prompt_yes_no(const char *prompt) {
  if (prompt) {
    fputs(prompt, stderr);
    fflush(stderr);
  }
  char buf[32];
  if (!fgets(buf, sizeof(buf), stdin)) {
    return 0;
  }
  for (size_t i = 0; buf[i] != '\0'; i++) {
    if (isspace((unsigned char)buf[i])) {
      continue;
    }
    return (buf[i] == 'y' || buf[i] == 'Y');
  }
  return 0;
}

/* malloc que nunca devolve NULL: em caso de falha termina o programa. */
void *xmalloc(size_t n) {
  void *p = malloc(n ? n : 1);
  if (!p) {
    perror("malloc");
    exit(1);
  }
  return p;
}

/* realloc que nunca devolve NULL: em caso de falha termina o programa. */
void *xrealloc(void *p, size_t n) {
  void *r = realloc(p, n ? n : 1);
  if (!r) {
    perror("realloc");
    exit(1);
  }
  return r;
}

/* Copia uma string para memória nova usando xmalloc (NULL se s==NULL). */
char *xstrdup(const char *s) {
  if (!s) {
    return NULL;
  }
  size_t n = strlen(s);
  char *d = xmalloc(n + 1);
  memcpy(d, s, n + 1);
  return d;
}

/*
 * write_all tenta escrever todos os bytes pedidos.
 * Útil para lidar com writes parciais e interrupções (EINTR).
 */
ssize_t write_all(int fd, const void *buf, size_t n) {
  const unsigned char *p = (const unsigned char *)buf;
  size_t off = 0;
  while (off < n) {
    ssize_t w = write(fd, p + off, n - off);
    if (w < 0) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    off += (size_t)w;
  }
  return (ssize_t)off;
}

/*
 * Junta dois caminhos "a" e "b" numa string nova.
 * Se "a" acabar sem '/', adiciona-o automaticamente.
 */
char *path_join2(const char *a, const char *b) {
  if (!a || !*a) {
    return xstrdup(b ? b : "");
  }
  if (!b || !*b) {
    return xstrdup(a);
  }
  size_t alen = strlen(a);
  size_t blen = strlen(b);
  int need_slash = (a[alen - 1] != '/');
  size_t n = alen + (need_slash ? 1 : 0) + blen + 1;
  char *p = xmalloc(n);
  memcpy(p, a, alen);
  size_t pos = alen;
  if (need_slash) {
    p[pos++] = '/';
  }
  memcpy(p + pos, b, blen);
  p[pos + blen] = '\0';
  return p;
}
