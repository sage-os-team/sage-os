#ifndef __SHELL_H__
#define __SHELL_H__

#include <thread.h>

#define FUNC(name) void name(task_t *proc, char *arg, char *pwd, char *ret)

typedef struct cmd {
  const char *name;
  void (*func)(task_t *proc, char *arg, char *pwd, char *ret);
} cmd_t;

void shell_init();
bool get_dir(char *arg, char *pwd, char *dir);
int fork1();
void runcmd(task_t *proc, char *arg, char *pwd, char *ret, int i);

FUNC(man);
FUNC(echo);
FUNC(pwd);
FUNC(ls);
FUNC(cd);
FUNC(cat);
FUNC(write);
FUNC(link);
FUNC(mkdir);
FUNC(rmdir);
FUNC(rm);
FUNC(ps);
FUNC(stat);
FUNC(mem);

#endif