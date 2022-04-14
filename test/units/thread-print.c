#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <spinlock.h>
#include <thread.h>
#include <logger.h>

#define MAX_TASK      4
#define RUNNING_COUNT 3

task_t tasks[MAX_TASK];
char names[10][MAX_TASK];
int ids[MAX_TASK];

spinlock_t print_lock;

void func(void *arg) {
  int id = *(int *)arg;
  for (int i = 0; i < RUNNING_COUNT; i++) {
    // every thread run once, and then yield
    spin_lock(&print_lock);
    info("T%03d running %03d times", id, i + 1);
    kmt_print_cpu_tasks();
    printf("\n");
    spin_unlock(&print_lock);
    yield();
  }
  spin_lock(&print_lock);
  success("T%03d exit!", id);
  kmt_print_all_tasks();
  kmt_print_cpu_tasks();
  spin_unlock(&print_lock);
  if (id < cpu_count())
    while (1)
      ;
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
  create_threads();
  kmt_print_all_tasks();
  kmt_print_cpu_tasks();
  mpe_init(os->run);
  return 1;
}