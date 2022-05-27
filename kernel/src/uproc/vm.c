#include <kernel.h>
#include <thread.h>
#include <buddy.h>
#include <user.h>

void uproc_pgmap(task_t* proc, void* vaddr, void* paddr, int prot);
void uporc_pgunmap(task_t* proc, void* vaddr);
void inituvm(task_t* proc, unsigned char* init, int sz);
int allocuvm(task_t* proc, int newsz, int oldsz);
void copyuvm(task_t* proc, task_t* src, int sz);
int deallocuvm(task_t* proc, int newsz, int oldsz);

/**
 * @brief map one physical page to one virtual page.
 *
 * @param proc PCB
 * @param vaddr virtual page addr
 * @param paddr physical page addr
 * @param prot protection bit
 */
void uproc_pgmap(task_t* proc, void* vaddr, void* paddr, int prot) {
  if (prot == MMAP_NONE) {
    error("Try to unmap a vaddr in mapping function");
    return;
  }
  uintptr_t va   = (uintptr_t)vaddr;
  mapnode_t* pos = NULL;
  list_for_each_entry(pos, &proc->pg_map, list) {
    if ((intptr_t)pos->va == va) {
      error("try to map mapping va");
      return;
    }
  }
  // need to introduce slab system, otherwise the waste of space is huge.
  mapnode_t* node = pmm->alloc(sizeof(mapnode_t));
  node->pa        = paddr;
  node->va        = vaddr;
  list_add(&node->list, &proc->pg_map);
  AddrSpace* as = &proc->as;
  info("AS %x map va:0x%06x%06x -> pa:0x%x", as->ptr, va >> 24,
       va & ((1L << 24) - 1), paddr);
  // function map already has checks
  map(as, vaddr, paddr, prot);
}

/**
 * @brief unmap one virtual page
 *
 * @param as
 * @param vaddr
 */
void uporc_pgunmap(task_t* proc, void* vaddr) {
  uintptr_t va   = (uintptr_t)vaddr;
  mapnode_t* pos = NULL;
  int flag       = 0;
  list_for_each_entry(pos, &proc->pg_map, list) {
    if ((intptr_t)pos->va == (intptr_t)vaddr) {
      flag = 1;
      break;
    }
  }
  if (!flag) {
    panic("try to unmap a empty page frame");
  }
  AddrSpace* as = &proc->as;
  info("AS %x unmap va:0x%06x%06x", as->ptr, va >> 24, va & ((1L << 24) - 1));
  map(as, vaddr, 0, MMAP_NONE);
}

void inituvm(task_t* proc, unsigned char* init, int sz) {
  AddrSpace* as = &proc->as;
  void* va      = as->area.start;
  int i;
  char* mem;
  for (i = 0; i < sz; i += SZ_PAGE) {
    mem = pmm->pgalloc();
    memset(mem, 0, SZ_PAGE);
    if (i + SZ_PAGE <= sz) {
      memcpy(mem, (char*)(intptr_t)(init + i), SZ_PAGE);
    } else {
      memcpy(mem, (char*)(intptr_t)(init + i), sz - i);
    }
    uproc_pgmap(proc, va + i, mem, MMAP_READ | MMAP_WRITE);
  }
}

int allocuvm(task_t* proc, int newsz, int oldsz) {
  // AddrSpace* as = &proc->as;
  assert(newsz >= oldsz);

  uintptr_t a = ROUNDUP(oldsz, SZ_PAGE);
  char* mem;
  for (; a < newsz; a += SZ_PAGE) {
    mem = pmm->pgalloc();
    if (mem == 0) {
      panic("memory overflowed");
    }
    memset(mem, 0, SZ_PAGE);
    uproc_pgmap(proc, (void*)a, mem, MMAP_READ | MMAP_WRITE);
  }
  return newsz;
}

void copyuvm(task_t* proc, task_t* src, int sz) {
  // intptr_t i;
  char* a;
  mapnode_t* pos = NULL;
  list_for_each_entry(pos, &src->pg_map, list) {
    a = pmm->pgalloc();
    memcpy(a, (char*)pos->pa, SZ_PAGE);
    uproc_pgmap(proc, pos->va, a,
                MMAP_READ | MMAP_WRITE);  // only read & write
  }
}

int deallocuvm(task_t* proc, int newsz, int oldsz) {
  assert(newsz <= oldsz);
  panic("not implemented");
}