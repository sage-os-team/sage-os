/**
 * @file thread-spin-braces.c
 * @author moeakwak (moeakwak@gmail.com)
 * @brief producer-consumer demo with spinlock
 * @version 0.2
 * @date 2022-04-15
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <spinlock.h>
#include <thread.h>
#include <logger.h>
#include <devices.h>

#define MAX_COUNT 60

task_t *task_consumer, *task_producer;

bool stop = false;
int cnt   = 0;
spinlock_t lock, print_lock;

// consume 2 left braces one time
void consumer(void *arg) {
  task_t *self = (task_t *)arg;
  while (1) {
    spin_lock(&lock);
    bool _stop = stop;
    if (cnt >= 2) {
      cnt -= 2;
      spin_lock(&print_lock);
      cprintf("tty1", "))");
      spin_unlock(&print_lock);
    }
    spin_unlock(&lock);

    if (cnt == 0 && _stop) {
      spin_lock(&print_lock);
      cprintf("tty1", "C");
      printf("***** CPU %d: Consumer finished! *****\n", cpu_current());
      kmt_print_all_tasks(LOG_INFO);
      spin_unlock(&print_lock);
      _log_mask = LOG_ERROR | LOG_INFO;
      kmt->teardown(self);
      while (1)
        ;
    }
    for (int i = 0; i < 10000000; i++)
      ;
  }
}

// produce 3 left brace one time
void producer(void *arg) {
  task_t *self = (task_t *)arg;
  int tot      = 0;
  while (1) {
    spin_lock(&lock);
    cnt += 3;
    tot += 3;
    spin_lock(&print_lock);
    cprintf("tty1", "(((");
    spin_unlock(&print_lock);
    spin_unlock(&lock);
    if (tot >= MAX_COUNT) {
      break;
    }
  }
  spin_lock(&lock);
  stop = true;
  spin_lock(&print_lock);
  cprintf("tty1", "P");
  printf("***** CPU %d: Producer finished! *****\n", cpu_current());
  kmt_print_all_tasks(LOG_INFO);
  spin_unlock(&print_lock);
  spin_unlock(&lock);

  kmt->teardown(self);
  while (1)
    ;
}

static void create_threads() {
  task_consumer = pmm->alloc(sizeof(task_t));
  task_producer = pmm->alloc(sizeof(task_t));
  kmt->create(task_consumer, "consumer", consumer, task_consumer);
  kmt->create(task_producer, "producer", producer, task_producer);
}

int main() {
  ioe_init();

  _log_mask = LOG_ERROR | LOG_WARN | LOG_INFO | LOG_SUCCESS;

  cte_init(os->trap);
  os->init();

  spin_init(&print_lock, "print_lock");
  spin_init(&lock, "lock");

  create_threads();

  kmt_print_all_tasks(LOG_INFO);
  kmt_print_cpu_tasks(LOG_INFO);
  mpe_init(os->run);
  return 1;
}