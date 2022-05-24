#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#include <thread.h>

Context *syscall_handler(Event ev, Context *context);

/* uproc syscalls */

int sys_kputc(task_t *proc, char ch);
int sys_fork(task_t *proc);
int sys_wait(task_t *proc, int *status);
int sys_exit(task_t *proc, int status);
int sys_kill(task_t *proc, int pid);
void *sys_mmap(task_t *proc, void *addr, int length, int prot, int flags);
int sys_getpid(task_t *proc);
int sys_sleep(task_t *proc, int seconds);
int64_t sys_uptime(task_t *proc);
int sys_sbrk(int n);

/* vfs syscalls */

int sys_open(task_t *proc, const char *pathname, int flags);
int sys_close(task_t *proc, int fd);
int sys_read(task_t *proc, int fd, void *buf, size_t nbyte);
int sys_write(task_t *proc, int fd, void *buf, size_t nbyte);
// int sys_lseek(task_t *proc, int fd, int offset, int whence);
int sys_link(task_t *proc, const char *oldpath, const char *newpath);
int sys_unlink(task_t *proc, const char *pathname);
int sys_fstat(task_t *proc, int fd, stat_t *buf);
int sys_mkdir(task_t *proc, const char *pathname);
int sys_chdir(task_t *proc, const char *path);
int sys_dup(task_t *proc, int fd);

#endif