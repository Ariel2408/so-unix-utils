#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int arg_has_flag(int argc, char **argv, const char *flag) {
  for (int i = 1; i < argc; i++) {
    if (argv[i] && strcmp(argv[i], flag) == 0) {
      return 1;
    }
  }
  return 0;
}

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

void *xmalloc(size_t n) {
  void *p = malloc(n ? n : 1);
  if (!p) {
    perror("malloc");
    exit(1);
  }
  return p;
}

void *xrealloc(void *p, size_t n) {
  void *r = realloc(p, n ? n : 1);
  if (!r) {
    perror("realloc");
    exit(1);
  }
  return r;
}

char *xstrdup(const char *s) {
  if (!s) {
    return NULL;
  }
  size_t n = strlen(s);
  char *d = xmalloc(n + 1);
  memcpy(d, s, n + 1);
  return d;
}

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
