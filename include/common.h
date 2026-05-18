#ifndef COMMON_H
#define COMMON_H

#include <stddef.h>
#include <sys/types.h>

int arg_has_flag(int argc, char **argv, const char *flag);
int parse_dash_number(const char *arg, long *out);
long parse_long_strict(const char *s, int *ok);

int prompt_yes_no(const char *prompt);

void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);

ssize_t write_all(int fd, const void *buf, size_t n);
char *path_join2(const char *a, const char *b);

#endif
