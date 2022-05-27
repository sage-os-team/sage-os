#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <spinlock.h>
#include <thread.h>
#include <logger.h>
#include <syscalls.h>

#define MAX_TASK      4
#define RUNNING_COUNT 2

task_t *tasks[MAX_TASK], *print_task;
char names[10][MAX_TASK];

int cnt = MAX_TASK;  // current running thread count
spinlock_t cnt_lock, print_lock;

void func(void *arg) {
  while (1) {
  }
}

void print_func(void *arg) {
  while (1) {
    sys_sleep(current_task, 1);
    yield();
    kmt_print_all_tasks(LOG_INFO);
  }
}

static void create_threads() {
  for (int i = 0; i < MAX_TASK; i++) {
    tasks[i] = pmm->alloc(sizeof(task_t));
    sprintf(names[i], "T%02d\0", i);
    kmt->create(tasks[i], names[i], func, NULL);
  }
  print_task = pmm->alloc(sizeof(task_t));
  kmt->create(print_task, "print_task", print_func, NULL);
}

int main() {
  ioe_init();

  _log_mask = LOG_INFO | LOG_WARN | LOG_ERROR | LOG_SUCCESS;

  cte_init(os->trap);
  os->init();

  spin_init(&print_lock, "print_lock");
  spin_init(&cnt_lock, "cnt_lock");

  create_threads();
  kmt_print_all_tasks(LOG_INFO);
  kmt_print_cpu_tasks(LOG_INFO);
  mpe_init(os->run);
  return 1;
}