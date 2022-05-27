#ifndef __IO_H__
#define __IO_H__

#include <spinlock.h>
#include <klib-macros.h>

// see amdev.h
#define AMDEV_CNT 24
spinlock_t io_locks[AMDEV_CNT + 1];
char amdev_name[AMDEV_CNT][16];

void io_init();

#define safe_io_read(reg) \
  ({ \
    spin_lock(&io_locks[(reg)]); \
    reg##_T __io_param; \
    ioe_read(reg, &__io_param); \
    spin_unlock(&io_locks[(reg)]); \
    __io_param; \
  })

#define safe_io_write(reg, ...) \
  ({ \
    spin_lock(&io_locks[(reg)]); \
    reg##_T __io_param = (reg##_T){__VA_ARGS__}; \
    spin_unlock(&io_locks[(reg)]); \
    ioe_write(reg, &__io_param); \
  })

#endif