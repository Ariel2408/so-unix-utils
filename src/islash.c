#include "common.h"

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void print_help(FILE *out) {
  fprintf(out,
          "UEsh (islash) - shell / interpretador de comandos\n"
          "Sintaxe: islash\n"
          "Internos: cd, pwd, echo, sleep, exit\n"
          "Externos: ls, sort, cp, mv, rm, grep, head, tail\n"
          "Opcoes:\n"
          "  -h        mostra esta ajuda e sai\n");
}

static void prompt(void) {
  const char *user = NULL;
  struct passwd *pw = getpwuid(getuid());
  if (pw && pw->pw_name) {
    user = pw->pw_name;
  } else {
    user = "user";
  }

  char host[256];
  if (gethostname(host, sizeof(host)) != 0) {
    strncpy(host, "host", sizeof(host));
    host[sizeof(host) - 1] = '\0';
  }

  char cwd[4096];
  if (!getcwd(cwd, sizeof(cwd))) {
    strncpy(cwd, "?", sizeof(cwd));
    cwd[sizeof(cwd) - 1] = '\0';
  }

  printf("%s@%s (%s) > ", user, host, cwd);
  fflush(stdout);
}

static char **tokenize(char *line, int *out_argc) {
  char **args = NULL;
  size_t used = 0;
  size_t cap = 0;

  char *save = NULL;
  for (char *tok = strtok_r(line, " \t\r\n", &save); tok;
       tok = strtok_r(NULL, " \t\r\n", &save)) {
    if (used == cap) {
      cap = cap ? cap * 2 : 16;
      args = xrealloc(args, cap * sizeof(*args));
    }
    args[used++] = tok;
  }

  if (used == cap) {
    cap = cap ? cap + 1 : 1;
    args = xrealloc(args, cap * sizeof(*args));
  }
  args[used] = NULL;
  if (out_argc) {
    *out_argc = (int)used;
  }
  return args;
}

static int is_project_cmd(const char *cmd) {
  static const char *names[] = {"ls",   "sort", "cp",   "mv",   "rm",
                                "grep", "head", "tail", NULL};
  for (size_t i = 0; names[i]; i++) {
    if (strcmp(cmd, names[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

static void run_external(const char *start_dir, char **args) {
  pid_t pid = fork();
  if (pid < 0) {
    fprintf(stderr, "islash: fork: %s\n", strerror(errno));
    return;
  }

  if (pid == 0) {
    const char *cmd = args[0];
    if (cmd && strchr(cmd, '/') == NULL && is_project_cmd(cmd)) {
      char *bin_dir = path_join2(start_dir, "bin");
      char *full = path_join2(bin_dir, cmd);
      execv(full, args);
      free(full);
      free(bin_dir);
    }

    execvp(cmd, args);
    fprintf(stderr, "islash: %s: %s\n", cmd, strerror(errno));
    _exit(127);
  }

  int status = 0;
  while (waitpid(pid, &status, 0) < 0) {
    if (errno == EINTR) {
      continue;
    }
    break;
  }
}

int main(int argc, char **argv) {
  if (arg_has_flag(argc, argv, "-h")) {
    print_help(stdout);
    return 0;
  }

  char start_dir[4096];
  if (!getcwd(start_dir, sizeof(start_dir))) {
    strncpy(start_dir, ".", sizeof(start_dir));
    start_dir[sizeof(start_dir) - 1] = '\0';
  }

  char *line = NULL;
  size_t cap = 0;

  for (;;) {
    prompt();
    errno = 0;
    ssize_t r = getline(&line, &cap, stdin);
    if (r < 0) {
      break;
    }

    int ac = 0;
    char **args = tokenize(line, &ac);
    if (ac == 0) {
      free(args);
      continue;
    }

    const char *cmd = args[0];
    if (strcmp(cmd, "exit") == 0) {
      free(args);
      break;
    }

    if (strcmp(cmd, "cd") == 0) {
      const char *dir = (ac >= 2) ? args[1] : getenv("HOME");
      if (!dir) {
        dir = ".";
      }
      if (chdir(dir) != 0) {
        fprintf(stderr, "cd: %s: %s\n", dir, strerror(errno));
      }
      free(args);
      continue;
    }

    if (strcmp(cmd, "pwd") == 0) {
      char cwd[4096];
      if (getcwd(cwd, sizeof(cwd))) {
        puts(cwd);
      } else {
        fprintf(stderr, "pwd: %s\n", strerror(errno));
      }
      free(args);
      continue;
    }

    if (strcmp(cmd, "echo") == 0) {
      for (int i = 1; i < ac; i++) {
        fputs(args[i], stdout);
        if (i + 1 < ac) {
          fputc(' ', stdout);
        }
      }
      fputc('\n', stdout);
      free(args);
      continue;
    }

    if (strcmp(cmd, "sleep") == 0) {
      if (ac != 2) {
        fprintf(stderr, "sleep: argumentos invalidos\n");
        free(args);
        continue;
      }
      int ok = 0;
      long secs = parse_long_strict(args[1], &ok);
      if (!ok || secs < 0) {
        fprintf(stderr, "sleep: numero invalido\n");
        free(args);
        continue;
      }
      sleep((unsigned int)secs);
      free(args);
      continue;
    }

    run_external(start_dir, args);
    free(args);
  }

  free(line);
  return 0;
}
