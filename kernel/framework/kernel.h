#ifndef __KERNEL_H__
#define __KERNEL_H__
#include <am.h>

#define MODULE(mod) \
  typedef struct mod_##mod##_t mod_##mod##_t; \
  extern mod_##mod##_t *mod; \
  struct mod_##mod##_t

#define MODULE_DEF(mod) \
  extern mod_##mod##_t __##mod##_obj; \
  mod_##mod##_t *mod = &__##mod##_obj; \
  mod_##mod##_t __##mod##_obj

typedef Context *(*handler_t)(Event, Context *);
MODULE(os) {
  void (*init)();
  void (*run)();
  Context *(*trap)(Event ev, Context * context);
  void (*on_irq)(int seq, int event, handler_t handler);
};

MODULE(pmm) {
  void (*init)();
  void *(*alloc)(size_t size);
  void (*free)(void *ptr);
  void *(*pgalloc)();
};

typedef struct task task_t;
typedef struct spinlock spinlock_t;
typedef struct semaphore sem_t;

MODULE(kmt) {
  void (*init)();
  int (*create)(task_t * task, const char *name, void (*entry)(void *arg),
                void *arg);
  void (*teardown)(task_t * task);
  task_t *(*get_task)();
  void (*set_task)(task_t * task);
  void (*spin_init)(spinlock_t * lk, const char *name);
  void (*spin_lock)(spinlock_t * lk);
  void (*spin_unlock)(spinlock_t * lk);
  void (*sem_init)(sem_t * sem, const char *name, int value);
  void (*sem_wait)(sem_t * sem);
  void (*sem_signal)(sem_t * sem);
};

typedef struct device device_t;
MODULE(dev) {
  void (*init)();
  device_t *(*lookup)(const char *name);
};

MODULE(uproc) {
  void (*init)();
  int (*kputc)(task_t * task, char ch);
  int (*fork)(task_t * task);
  int (*wait)(task_t * task, int *status);
  int (*exit)(task_t * task, int status);
  int (*kill)(task_t * task, int pid);
  void *(*mmap)(task_t * task, void *addr, int length, int prot, int flags);
  int (*getpid)(task_t * task);
  int (*sleep)(task_t * task, int seconds);
  int64_t (*uptime)(task_t * task);
};

typedef struct dinode dinode_t;
typedef struct block block_t;
typedef struct dirent dirent_t;
typedef struct superblock superblock_t;
typedef struct inode inode_t;
typedef struct file_stat stat_t;
typedef struct file file_t;

MODULE(vfs) {
  void (*init)();
  int (*write)(task_t *proc, int fd, void *buf, size_t count);
  int (*read)(task_t *proc, int fd, void *buf, size_t count);
  int (*close)(task_t *proc, int fd);
  int (*open)(task_t *proc, const char *pathname, int flags);
  int (*link)(task_t *proc, const char *oldpath, const char *newpath);
  int (*unlink)(task_t *proc, const char *pathname);
  int (*fstat)(task_t *proc, int fd, stat_t *buf);
  int (*mkdir)(task_t *proc, const char *pathname);
  int (*chdir)(task_t *proc, const char *path);
  int (*dup)(task_t *proc, int fd);
};

MODULE(fs) {
  void (*init)();
  void (*readblk)(device_t* dev, uint32_t blk_no, block_t* buf);
  void (*writeblk)(device_t* dev, uint32_t blk_no, block_t* buf);
  void (*zeroblk)(device_t* dev, uint32_t blk_no);
  uint32_t (*allocblk)(device_t* dev);
  void (*freeblk)(device_t* dev, uint32_t blk_no);
  void (*readinode)(device_t* dev, uint32_t inode_no, inode_t* inode);
  void (*writeinode)(device_t* dev, uint32_t inode_no, inode_t* inode);
};

// MODULE(file) {
//   void (*init)(void);
//   int (*alloc)();
//   int (*dup)(file_t* f);
//   int (*stat)(file_t* f, stat_t* st);
//   void (*close)(file_t* f);
//   int (*read)(file_t* f, char* buf, uint32_t n);
//   int (*write)(file_t* f, char* buf, uint32_t n);
//   file_t* (*get)(uint32_t fd);
// };

#endif