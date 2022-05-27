#include <vfs.h>
#include <kernel.h>
#include <syscall_defs.h>
#include <syscalls.h>
#include <thread.h>

/**
 * @brief EVENT_SYSCALL handler on trap
 *
 * @param ev
 * @param context
 * @return Context*
 */
Context *syscall_handler(Event ev, Context *context) {
  task_t *proc     = current_task;
  uint64_t args[4] = {context->rdi, context->rsi, context->rdx, context->rcx};
  uint64_t retval  = 0;
  int sys_id       = context->rax;
  switch (sys_id) {
    case SYS_kputc:
      retval = sys_kputc(proc, args[0]);
      break;
    case SYS_fork:
      retval = sys_fork(proc);
      break;
    case SYS_exit:
      retval = sys_exit(proc, args[0]);
      break;
    case SYS_wait:
      retval = sys_wait(proc, (int *)args[0]);
      break;
    case SYS_read:
      retval = sys_read(proc, args[0], (void *)args[1], args[2]);
      break;
    case SYS_kill:
      retval = sys_kill(proc, args[0]);
      break;
    case SYS_getpid:
      retval = sys_getpid(proc);
      break;
    case SYS_mmap:
      sys_mmap(proc, (void *)args[0], args[1], args[2], args[3]);
      retval = context->rax;
      break;
    case SYS_sleep:
      retval = sys_sleep(proc, args[0]);
      break;
    case SYS_uptime:
      retval = sys_uptime(proc);
      break;
    case SYS_fstat:
      retval = sys_fstat(proc, args[0], (void *)args[1]);
      break;
    case SYS_chdir:
      retval = sys_chdir(proc, (void *)args[0]);
      break;
    case SYS_dup:
      retval = sys_dup(proc, args[0]);
      break;
    case SYS_open:
      retval = sys_open(proc, (void *)args[0], args[1]);
      break;
    case SYS_write:
      retval = sys_write(proc, args[0], (void *)args[1], args[2]);
      break;
    case SYS_unlink:
      retval = sys_unlink(proc, (void *)args[0]);
      break;
    case SYS_link:
      retval = sys_link(proc, (void *)args[0], (void *)args[1]);
      break;
    case SYS_mkdir:
      retval = sys_mkdir(proc, (void *)args[0]);
      break;
    case SYS_close:
      retval = sys_close(proc, args[0]);
      break;
    default:
      assert_msg(false, "syscall not implemented: %d", sys_id);
      break;
  }
  context->rax = retval;
  return NULL;
}
