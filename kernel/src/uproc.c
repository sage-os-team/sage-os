#include <kernel.h>
#include <syscall_defs.h>
#include <thread.h>

#include "initcode.inc"

void uproc_init();
int uproc_create(task_t *proc, const char *name);
void uproc_pgmap(task_t *proc, void *vaddr, void *paddr, int prot);
Context *uproc_pagefault(Event ev, Context *context);
Context *uproc_syscall(Event ev, Context *context);
int sys_kputc(task_t *proc, char ch);
int sys_fork(task_t *proc);
int sys_wait(task_t *proc, int *status);
int sys_exit(task_t *proc, int status);
int sys_kill(task_t *proc, int pid);
void *sys_mmap(task_t *proc, void *addr, int length, int prot, int flags);
int sys_getpid(task_t *proc);
int sys_sleep(task_t *proc, int seconds);
int64_t sys_uptime(task_t *proc);

/**
 * @brief initialize uproc
 *
 */
void uproc_init() {
  task_t *initproc = pmm->alloc(sizeof(task_t));
  uproc_create(initproc, "initcode");
  os->on_irq(0, EVENT_SYSCALL, uproc_syscall);
  os->on_irq(0, EVENT_PAGEFAULT, uproc_pagefault);
}

/**
 * @brief create a user process.
 *        Notice: copy _init in one page
 *
 * @param proc PCB
 * @param name process name
 */
int uproc_create(task_t *proc, const char *name) {
  assert_msg(proc != NULL && name != NULL, "null arguments in uproc_create");
  proc->pid      = kmt_next_pid();
  proc->name     = name;
  proc->entry    = NULL;
  proc->arg      = NULL;
  proc->state    = ST_E;
  proc->owner    = -1;
  proc->count    = 0;
  proc->wait_sem = NULL;
  proc->killed   = 0;
  proc->next     = NULL;

  memset(proc->fenceA, FILL_FENCE, sizeof(proc->fenceA));
  memset(proc->stack, FILL_STACK, sizeof(proc->stack));
  memset(proc->fenceB, FILL_FENCE, sizeof(proc->fenceB));

  Area kstack = {(void *)proc->stack,
                 (void *)proc->stack + sizeof(proc->stack)};
  protect(&proc->as);
  AddrSpace *as = &proc->as;

  // ucontext entry: start addr of proc
  proc->context = ucontext(as, kstack, as->area.start);

  // copy init code
  void *paddr = pmm->pgalloc();
  void *vaddr = as->area.start;
  memcpy(paddr, _init, _init_len);
  uproc_pgmap(proc, vaddr, paddr, MMAP_READ | MMAP_WRITE);

  // add to task list
  assert_msg(!spin_holding(&task_list_lock), "already hold task_list_lock");
  kmt->spin_lock(&task_list_lock);
  task_t *tp = &root_task;
  while (tp->next)
    tp = tp->next;
  tp->next = proc;
  kmt->spin_unlock(&task_list_lock);

  return proc->pid;
}

/**
 * @brief map one physical page to one virtual page.
 *
 * @param proc PCB
 * @param vaddr virtual page addr
 * @param paddr physical page addr
 * @param prot protection bit
 */
void uproc_pgmap(task_t *proc, void *vaddr, void *paddr, int prot) {
  // TODO: need to record mapped pages for proc?
  uintptr_t va = (uintptr_t)vaddr;
  info("map va:0x%06x%06x -> pa:0x%x", va >> 24, va & ((1L << 24) - 1), paddr);
  // function map already has checks
  map(&proc->as, vaddr, paddr, prot);
}

/**
 * @brief EVENT_PAGEFAULT handler on trap
 *
 * @param ev
 * @param context
 * @return Context* NULL
 */
Context *uproc_pagefault(Event ev, Context *context) {
  // ev.ref: the failed virtual addr, 48bit unsigned long
  uintptr_t ref = (uintptr_t)ev.ref;
  info("pagefault: %06x%06x", ref >> 24, (uint32_t)ref & ((1L << 24) - 1));
  AddrSpace *as = &cpu_tasks[cpu_current()]->as;
  void *paddr   = pmm->pgalloc();
  // vaddr:  the start addr of that page
  uintptr_t vaddr = ref & ~(as->pgsize - 1L);
  uproc_pgmap(cpu_tasks[cpu_current()], (void *)vaddr, paddr,
              MMAP_READ | MMAP_WRITE);
  return NULL;
}

/**
 * @brief EVENT_SYSCALL handler on trap
 *
 * @param ev
 * @param context
 * @return Context*
 */
Context *uproc_syscall(Event ev, Context *context) {
  task_t *proc     = cpu_tasks[cpu_current()];
  uint64_t args[4] = {context->rdi, context->rsi, context->rdx, context->rcx};
  uint64_t rax     = 0;
  int sys_id       = context->rax;
  switch (sys_id) {
    case SYS_kputc:
      rax = sys_kputc(proc, args[0]);
      break;
    case SYS_fork:
      rax = sys_fork(proc);
      break;
    case SYS_exit:
      rax = sys_exit(proc, args[0]);
      break;
    case SYS_wait:
      rax = sys_wait(proc, (int *)args[0]);
      break;
    case SYS_kill:
      rax = sys_kill(proc, args[0]);
      break;
    case SYS_getpid:
      rax = sys_getpid(proc);
      break;
    case SYS_mmap:
      sys_mmap(proc, (void *)args[0], args[1], args[2], args[3]);
      break;
    case SYS_sleep:
      rax = sys_sleep(proc, args[0]);
      break;
    case SYS_uptime:
      rax = sys_uptime(proc);
      break;
    default:
      assert_msg(false, "syscall not implemented: %d", sys_id);
      break;
  }
  if (sys_id != SYS_mmap) context->rax = rax;
  return NULL;
}

int sys_kputc(task_t *proc, char ch) {
  putch(ch);
  return 0;
}

int sys_fork(task_t *proc) {
  task_t *subproc = pmm->alloc(sizeof(task_t));
  // TODO: change subproc's name
  uproc_create(subproc, proc->name);

  // do not copy parent's rsp0, cr3
  uintptr_t rsp0 = subproc->context->rsp0;
  void *cr3      = subproc->context->cr3;
  memcpy(subproc->context, proc->context, sizeof(Context));
  subproc->context->rsp0 = rsp0;
  subproc->context->cr3  = cr3;
  subproc->context->rax  = 0;

  // TODO: copy pages

  return subproc->pid;
}

int sys_exit(task_t *proc, int status) {
  panic("not implemented");
  return 1;
}

int sys_wait(task_t *proc, int *status) {
  panic("not implemented");
  return 1;
}

int sys_kill(task_t *proc, int pid) {
  panic("not implemented");
  return 1;
}

int sys_getpid(task_t *proc) {
  panic("not implemented");
  return 1;
}

void *sys_mmap(task_t *proc, void *addr, int length, int prot, int flags) {
  panic("not implemented");
}

int sys_sleep(task_t *proc, int seconds) {
#define TIMER_HZ 200
  proc->count = 0 - seconds * TIMER_HZ;
  proc->state = ST_S;
  return 0;
}

int64_t sys_uptime(task_t *proc) {
  return io_read(AM_TIMER_UPTIME).us / 1000;
}

MODULE_DEF(uproc) = {
    .init = uproc_init,
};