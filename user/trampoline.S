#include "../kernel/framework/syscall_defs.h"

.globl _start
_start:
  call main
  mov %rax, %rdi
  mov $SYS_exit, %rax
  int $0x80
