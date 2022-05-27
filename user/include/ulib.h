#ifndef ULIB_H__
#define ULIB_H__

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include "../../kernel/framework/syscall_defs.h"
#include "../../kernel/framework/user.h"

// string.h
void *memset(void *s, int c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
void *memmove(void *dst, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
size_t strlen(const char *s);
char *strcat(char *dst, const char *src);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, size_t n);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

// stdlib.h
void srand(unsigned int seed);
int rand(void);
void *malloc(size_t size);
void free(void *ptr);
int abs(int x);
int atoi(const char *nptr);

// stdio.h
int printf(const char *format, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int vsprintf(char *str, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

static inline long syscall(int num, long x1, long x2, long x3, long x4) {
  register long a0 asm("rax") = num;
  register long a1 asm("rdi") = x1;
  register long a2 asm("rsi") = x2;
  register long a3 asm("rdx") = x3;
  register long a4 asm("rcx") = x4;
  asm volatile("int $0x80"
               : "+r"(a0)
               : "r"(a1), "r"(a2), "r"(a3), "r"(a4)
               : "memory");
  return a0;
}

#define kputstr(s) \
  ({ \
    for (const char *p = s; *p; p++) \
      kputc(*p); \
  })

static inline int kputc(char ch) {
  return syscall(SYS_kputc, ch, 0, 0, 0);
}

static inline int fork() {
  return syscall(SYS_fork, 0, 0, 0, 0);
}

static inline int wait(int *status) {
  return syscall(SYS_wait, (uint64_t)status, 0, 0, 0);
}

static inline int exit(int status) {
  return syscall(SYS_exit, status, 0, 0, 0);
}

static inline int kill(int pid) {
  return syscall(SYS_kill, pid, 0, 0, 0);
}

static inline void *mmap(void *addr, int length, int prot, int flags) {
  return (void *)syscall(SYS_mmap, (uint64_t)addr, length, prot, flags);
}

static inline int getpid() {
  return syscall(SYS_getpid, 0, 0, 0, 0);
}

static inline int sleep(int seconds) {
  return syscall(SYS_sleep, seconds, 0, 0, 0);
}

static inline int64_t uptime() {
  return syscall(SYS_uptime, 0, 0, 0, 0);
}

static inline int open(const char *pathname, int flags) {
  return syscall(SYS_open, (uint64_t)pathname, flags, 0, 0);
}

static inline int close(int fd) {
  return syscall(SYS_close, fd, 0, 0, 0);
}

static inline int read(int fd, void *buf, size_t nbyte) {
  return syscall(SYS_read, fd, (uint64_t)buf, nbyte, 0);
}

static inline int write(int fd, void *buf, size_t nbyte) {
  return syscall(SYS_write, fd, (uint64_t)buf, nbyte, 0);
}

static inline int link(const char *oldpath, const char *newpath) {
  return syscall(SYS_link, (uint64_t)oldpath, (uint64_t)newpath, 0, 0);
}

static inline int unlink(const char *pathname) {
  return syscall(SYS_unlink, (uint64_t)pathname, 0, 0, 0);
}

static inline int fstat(int fd, _stat_t *buf) {
  return syscall(SYS_fstat, fd, (uint64_t)buf, 0, 0);
}

static inline int mkdir(const char *pathname) {
  return syscall(SYS_mkdir, (uint64_t)pathname, 0, 0, 0);
}

static inline int chdir(const char *path) {
  return syscall(SYS_chdir, (uint64_t)path, 0, 0, 0);
}

static inline int dup(int fd) {
  return syscall(SYS_dup, fd, 0, 0, 0);
}

#endif