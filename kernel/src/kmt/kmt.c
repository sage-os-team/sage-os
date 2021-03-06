#include <common.h>
#include <sem.h>
#include <spinlock.h>
#include <thread.h>
#include <syscalls.h>
#include <fs.h>

task_t *kmt_get_task();
void kmt_set_task(task_t *task);
Context *kmt_context_save(Event ev, Context *context);
Context *kmt_yield(Event ev, Context *context);
Context *kmt_error(Event ev, Context *context);
Context *kmt_timer(Event ev, Context *context);
Context *kmt_schedule(Event ev, Context *context);
void kmt_print_all_tasks(int mask);
void kmt_print_cpu_tasks(int mask);

uint32_t next_pid = 1;  // next pid to allocate

const char *task_states_str[] = {"Unused",   "Embryo",  "To sleep", "Sleeping",
                                 "Waken up", "Running", "Zombie",   "Special"};

const char fence_val[32] = {
    FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE,
    FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE,
    FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE,
    FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE,
    FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE, FILL_FENCE,
    FILL_FENCE, FILL_FENCE};

// use to lock whole os_trap
// extern spinlock_t ir_lock;

// use to lock task_list
spinlock_t task_list_lock = {
    .lock_flag = false, .name = "task_list_lock", .hold_cpuid = -1};

// special root point, do not execute
// all tasks are in root_task.list
task_t root_task;

// context to return when no task to schedule
Context *null_contexts[MAX_CPU] = {};

// current task in each cpu
// task_t *cpu_tasks[MAX_CPU] = {};
cpu_t percpu[MAX_CPU];

/**
 * @brief check the fence protection value.
 *
 * @param task
 */
#define CHECK_FENCE(task) \
  ({ \
    assert(memcmp(fence_val, (task)->fenceB, sizeof(fence_val)) == 0); \
    assert(memcmp(fence_val, (task)->fenceA, sizeof(fence_val)) == 0); \
  })

uint32_t kmt_next_pid() {
  assert(next_pid < 1 << 15);
  // TODO: reuse next pid
  return next_pid++;
}

/**
 * @brief initialize kmt module
 *
 */
void kmt_init() {
  // create root_task
  root_task.pid   = kmt_next_pid();
  root_task.name  = "Root Task";
  root_task.state = ST_X;
  root_task.count = 0;

  memset(root_task.fenceA, FILL_FENCE, sizeof(root_task.fenceA));
  memset(root_task.stack, FILL_STACK, sizeof(root_task.stack));
  memset(root_task.fenceB, FILL_FENCE, sizeof(root_task.fenceB));
  CHECK_FENCE(&root_task);
  root_task.next = NULL;

  os->on_irq(INT32_MIN, EVENT_NULL, kmt_context_save);
  os->on_irq(0, EVENT_ERROR, kmt_error);
  os->on_irq(0, EVENT_IRQ_TIMER, kmt_timer);
  os->on_irq(0, EVENT_YIELD, kmt_yield);
  os->on_irq(0, EVENT_SYSCALL, syscall_handler);
  os->on_irq(INT32_MAX, EVENT_NULL, kmt_schedule);
}

void kmt_init_task(task_t *task, const char *name, void (*entry)(void *arg),
                   void *arg) {
  assert_msg(task != NULL && name != NULL && entry != NULL,
             "null arguments in kmt_create");
  task->pid                 = kmt_next_pid();
  task->name                = name;
  task->entry               = entry;
  task->arg                 = arg;
  task->state               = ST_E;
  task->owner               = -1;
  task->priority            = 0;
  task->count               = 0;
  task->wait_sem            = NULL;
  task->killed              = 0;
  task->next                = NULL;
  task->as.ptr              = NULL;
  task->nctx                = 0;
  task->wait_subproc        = false;
  task->wait_subproc_status = NULL;
  task->fdtable[0]          = 0;
  task->fdtable[1]          = 1;
  task->fdtable[2]          = 2;
  task->cwd                 = iget(ROOTINO);
  for (int i = 3; i < PROCESS_FILE_TABLE_SIZE; i++)
    task->fdtable[i] = -1;

  memset(task->fenceA, FILL_FENCE, sizeof(task->fenceA));
  memset(task->stack, FILL_STACK, sizeof(task->stack));
  memset(task->fenceB, FILL_FENCE, sizeof(task->fenceB));

  Area stack = {(void *)task->stack, (void *)task->stack + sizeof(task->stack)};
  task->context[task->nctx++] = kcontext(stack, entry, arg);
  CHECK_FENCE(task);
  task->next = NULL;
}

/**
 * @brief create thread.
 *        Notice: cannot call create or teardown in irq handler!
 *
 * @param task task ptr of thread (should be allocated)
 * @param name thread name
 * @param entry thread user code function entry
 * @param arg args pass to entry
 * @return int task's pid
 */
int kmt_create(task_t *task, const char *name, void (*entry)(void *arg),
               void *arg) {
  assert_msg(!is_on_trap, "cannot create thread on trap!");

  kmt_init_task(task, name, entry, arg);

  assert_msg(!spin_holding(&task_list_lock), "already hold task_list_lock");
  spin_lock(&task_list_lock);
  task_t *tp = &root_task;
  while (tp->next)
    tp = tp->next;
  tp->next = task;
  spin_unlock(&task_list_lock);

  return task->pid;
}

/**
 * @brief tear down a thread
 *        Notice: cannot call create or teardown in irq handler!
 *
 * @param task task of thread
 */
void kmt_teardown(task_t *task) {
  assert_msg(!is_on_trap, "cannot teardown thread on trap!");
  // assert_msg(!spin_holding(&ir_lock), "do not allow kmt_teardown in trap");
  spin_lock(&task_list_lock);
  task->killed = 1;
  spin_unlock(&task_list_lock);
}

/**
 * @brief handler: save context to current task
 *
 * @param ev
 * @param context
 * @return Context* always NULL
 */
Context *kmt_context_save(Event ev, Context *context) {
  // assert(spin_holding(&ir_lock));
  task_t *cur = kmt_get_task();
  if (cur) {
    // assert(!cur->context);
    // TODO: more checks for context
    spin_lock(&task_list_lock);
    if (cur->state == ST_R) cur->state = ST_W;
    assert_msg(cur->nctx < CTX_STACK_SIZE, "context stack overflow!");
    cur->context[cur->nctx++] = context;
    spin_unlock(&task_list_lock);
  } else {
    // if no current task (initial), save to null_context
    assert(!null_contexts[cpu_current()]);
    null_contexts[cpu_current()] = context;
  }
  return NULL;
}

/**
 * @brief handler: handle yield interrupt (0x81)
 *
 * @param ev
 * @param context
 * @return Context* always NULL
 */
Context *kmt_yield(Event ev, Context *context) {
  spin_lock(&task_list_lock);
  task_t *cur = current_task;
  if (cur->wait_sem || cur->wait_subproc_status || cur->count < 0) {
    cur->state = ST_S;
  }
  spin_unlock(&task_list_lock);
  return NULL;
}

/**
 * @brief handler: schedule next task to run
 *        Notice: this should be the only handler which returns real Context
 *
 * @param ev
 * @param context
 * @return Context*
 */
Context *kmt_schedule(Event ev, Context *context) {
  // assert(spin_holding(&ir_lock));
  task_t *cur = kmt_get_task();

  // free killed process
  assert_msg(!spin_holding(&task_list_lock), "already hold task_list_lock");
  spin_lock(&task_list_lock);
  if (cur && cur->killed) {
    task_t *tp = &root_task;
    while (tp->next != cur)
      tp = tp->next;
    tp->next = cur->next;
    if (cur->as.ptr != NULL) {
      unprotect(&cur->as);
      // TODO: free uvm
    }
    pmm->free(cur);
  }

  // pick the next task to run
  Context *ret     = NULL;
  task_t *pick     = NULL;
  int min_priority = MAX_PRIORITY;

  for (task_t *tp = root_task.next; tp; tp = tp->next) {
    // CHECK_FENCE(tp);
    if (tp->owner != -1 && tp->owner != cpu_current()) continue;
    if (tp->state == ST_E || tp->state == ST_W) {
      assert_msg(tp->wait_sem == NULL,
                 "task %s can be sched, but has wait sem %s", tp->name,
                 tp->wait_sem->name);
      assert_msg(tp->wait_subproc == false,
                 "task %s can be sched, but has wait subproc", tp->name);
      if (pick == NULL || tp->priority < min_priority) {
        min_priority = tp->priority;
        pick         = tp;
      }
    }
  }

  // switch context
  if (pick != NULL) {
    pick->owner = cpu_current();
    CHECK_FENCE(pick);
    ret = pick->context[--pick->nctx];
    if (pick->nctx == 0) pick->state = ST_R;
    // tp->context[tp->nctx] = NULL;
    // if (tp->nctx > 0) tp->nctx--;
    pick->priority = (pick->priority + 1) % MAX_PRIORITY;

    // TODO: more checks here
    kmt_set_task(pick);
    if (ev.event != EVENT_IRQ_TIMER && ev.event != EVENT_YIELD)
      success("schedule: run next pid=%d, name=%s, count=%d, event=%d %s",
              pick->pid, pick->name, pick->count, ev.event, ev.msg);
  } else {
    // if no task to run
    // warn("schedule: no task to run");
    // kmt_print_all_tasks(LOG_WARN);
    // kmt_print_cpu_tasks(LOG_WARN);
    ret = null_contexts[cpu_current()];

    null_contexts[cpu_current()] = NULL;
    kmt_set_task(NULL);
  }

  spin_unlock(&task_list_lock);

  if (ret == NULL) {
    error_detail("switch to null context");
    kmt_print_all_tasks(LOG_ERROR);
    kmt_print_cpu_tasks(LOG_ERROR);
    panic("switch to null context");
  }
  return ret;
}

/**
 * @brief handler: triggered when aipc timer fired up
 *
 * @param ev
 * @param context
 * @return Context* always NULL
 */
Context *kmt_timer(Event ev, Context *context) {
  // wake up sleep process
  assert_msg(!spin_holding(&task_list_lock), "already hold task_list_lock");
  spin_lock(&task_list_lock);
  task_t *tp = NULL;
  for (tp = root_task.next; tp; tp = tp->next) {
    if (tp->count < 0) tp->count++;
    // wake up timer sleep tasks
    if (tp->state == ST_S && tp->count >= 0 && tp->wait_sem == NULL &&
        tp->wait_subproc == false) {
      tp->priority = 0;
      tp->state    = ST_W;
    }
  }
  spin_unlock(&task_list_lock);
  return NULL;
}

/**
 * @brief handler: handle program errors
 *
 * @param ev
 * @param context
 * @return Context* always NULL
 */
Context *kmt_error(Event ev, Context *context) {
  // assert(spin_holding(&ir_lock));
  assert(ev.event == EVENT_ERROR);
  error_detail("error detected: %s", ev.msg);
  // TODO: stop current task
  current_task->state = ST_Z;
  kmt_print_all_tasks(LOG_ERROR);
  kmt_print_cpu_tasks(LOG_ERROR);
  return NULL;
}

/**
 * @brief get current cpu's tasks. multi-thread safe.
 *
 * @return task_t*
 */
task_t *kmt_get_task() {
  assert(cpu_current() < MAX_CPU);
  return current_task;
}

/**
 * @brief set cpu's current task. multi-thread safe.
 *
 * @param task
 */
void kmt_set_task(task_t *task) {
  assert(cpu_current() < MAX_CPU);
  current_task = task;
}

/**
 * @brief print all tasks
 *        notice: do not garantee concurrency safety
 */
void kmt_print_all_tasks(int mask) {
  if (!(_log_mask & mask)) return;
  bool holding = spin_holding(&task_list_lock);
  if (!holding) spin_lock(&task_list_lock);
  printf("%s [all tasks]:\n", logger_type_str[mask]);
  for (task_t *tp = &root_task; tp != NULL; tp = tp->next) {
    printf("pid " FONT_BOLD "%d" FONT_NORMAL " <" FONT_BOLD "%.8s" FONT_NORMAL
           ">:\tcpu=" FONT_BOLD "%d" FONT_NORMAL ",\tnctx=%d,\tstate=" FONT_BOLD
           "%s" FONT_NORMAL ",\twait_sem=" FONT_BOLD "%s" FONT_NORMAL
           "\tpriority=" FONT_BOLD "%d," FONT_NORMAL "\tcount=" FONT_BOLD
           "%d" FONT_NORMAL "\n",
           tp->pid, tp->name, tp->owner, tp->nctx, task_states_str[tp->state],
           tp->wait_sem ? tp->wait_sem->name : "null", tp->priority, tp->count);
  }
  if (!holding) spin_unlock(&task_list_lock);
}

/**
 * @brief print all current_tasks
 *        notice: do not garantee concurrency safety
 */
void kmt_print_cpu_tasks(int mask) {
  if (!(_log_mask & mask)) return;
  bool holding = spin_holding(&task_list_lock);
  if (!holding) spin_lock(&task_list_lock);
  printf("%s [cpu tasks]:\n", logger_type_str[mask]);
  for (int i = 0; i < cpu_count(); i++) {
    task_t *tp = current_task;
    if (tp)
      printf("CPU %d pid %d <%s>:\t\tstate=%s, count=%d, wait_sem=%s\n", i,
             tp->pid, tp->name, task_states_str[tp->state], tp->count,
             tp->wait_sem ? tp->wait_sem->name : "null");
    else
      printf("#%d: empty\n", i);
  }
  if (!holding) spin_unlock(&task_list_lock);
}

MODULE_DEF(kmt) = {
    .init        = kmt_init,
    .create      = kmt_create,
    .teardown    = kmt_teardown,
    .get_task    = kmt_get_task,
    .set_task    = kmt_set_task,
    .spin_init   = spin_init,
    .spin_lock   = spin_lock,
    .spin_unlock = spin_unlock,
    .sem_init    = sem_init,
    .sem_signal  = sem_signal,
    .sem_wait    = sem_wait,
};
