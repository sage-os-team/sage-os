#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include <common.h>
#include <spinlock.h>
#include <thread.h>

struct semaphore {
  spinlock_t lock;     // spinlock of semaphore instance
  const char *name;    // name of semaphore instance
  volatile int value;  // cnt of semaphore instance
};

void sem_init(sem_t *sem, const char *name, int value);
void sem_wait(sem_t *sem);
void sem_signal(sem_t *sem);

#endif