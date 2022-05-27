#ifndef __COMMON_H__
#define __COMMON_H__

#include <am.h>
#include <amdev.h>
#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <logger.h>
#include <limits.h>

#define MAX_CPU 8  // defined in x86.h

#define KB      1024
#define SZ_PAGE (4 * KB)

static inline int power2ify(uint64_t size) {
  assert((int)size > 0);
  int order = 0;
  size--;
  do {
    size >>= 1;
    order++;
  } while (size);
  return order;
}

#endif
