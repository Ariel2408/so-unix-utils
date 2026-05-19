#ifndef COMMON_H
#define COMMON_H

/*
 * common.h
 * Helpers partilhados pelas utilidades deste projeto (parsing de argumentos,
 * alocação com falha fatal, e pequenas rotinas de I/O/caminhos).
 */

#include <stddef.h>
#include <sys/types.h>

/* Procura uma flag exata (ex.: "-h") na linha de comando. */
int arg_has_flag(int argc, char **argv, const char *flag);

/* Interpreta argumentos do tipo "-NUM" e escreve o valor em *out. */
int parse_dash_number(const char *arg, long *out);

/* Faz parsing estrito de um long (sem lixo no fim). Define *ok. */
long parse_long_strict(const char *s, int *ok);

/* Pergunta [y/N] no stdin. Devolve 1 para 'y'/'Y', 0 caso contrário. */
int prompt_yes_no(const char *prompt);

/* malloc/realloc com falha fatal (imprime erro e sai). */
void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);

/* strdup com falha fatal (NULL se s==NULL). */
char *xstrdup(const char *s);

/* Escreve exatamente n bytes, repetindo em caso de EINTR. */
ssize_t write_all(int fd, const void *buf, size_t n);

/* Junta dois componentes de caminho com '/' quando necessário. */
char *path_join2(const char *a, const char *b);

#endif
