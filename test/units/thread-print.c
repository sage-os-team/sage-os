#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <spinlock.h>
#include <thread.h>
#include <logger.h>

#define MAX_TASK      8
#define RUNNING_COUNT 3

task_t tasks[MAX_TASK];
char names[10][MAX_TASK];
int ids[MAX_TASK];

int cnt = 0;  // current running thread count
spinlock_t cnt_lock, print_lock;

void func(void *arg) {
  int id = *(int *)arg;

  spin_lock(&cnt_lock);
  cnt++;
  spin_unlock(&cnt_lock);

  for (int i = 0; i < RUNNING_COUNT; i++) {
    // print something
    spin_lock(&print_lock);
    warn("T%02d running %d times", id, i + 1);
    kmt_print_cpu_tasks();
    printf("\n");
    spin_unlock(&print_lock);
    // go to kernel; current cpu re-schedule task
    yield();
  }

  // if finished
  spin_lock(&print_lock);
  warn("T%03d exit!", id);
  kmt_print_all_tasks();
  kmt_print_cpu_tasks();
  spin_unlock(&print_lock);

  // hold cpu
  int remain = 0;
  spin_lock(&cnt_lock);
  remain = --cnt;
  spin_unlock(&cnt_lock);
  if (remain < cpu_count())
    while (1)
      ;
  yield();
}

static void create_threads() {
  for (int i = 0; i < MAX_TASK; i++) {
    sprintf(names[i], "T%02d\0", i);
    ids[i] = i;
    kmt->create(&tasks[i], names[i], func, &ids[i]);
  }
}

int main() {
  ioe_init();
  cte_init(os->trap);
  os->init();

  spin_init(&print_lock, "print_lock");
  spin_init(&cnt_lock, "cnt_lock");

  create_threads();
  kmt_print_all_tasks();
  kmt_print_cpu_tasks();
  mpe_init(os->run);
  return 1;
}