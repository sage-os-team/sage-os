#include <common.h>
#include <buddy.h>
#include <logger.h>
#include <slab.h>

#define _SIZE (1UL << SLAB_MAX_ORDER)



static void* kalloc(size_t size) {
  if (size <= _SIZE) {
    printf("small:alloc_in_slab\n");
		return alloc_in_slab(size);
	}
  printf("big:alloc in buddy system\n");
  int npage         = (size + 1) / SZ_PAGE;
  int acquire_order = power2ify(npage);
  log_detail(LOG_INFO, "acquire order: %d", acquire_order);
  struct chunk* page_addr = chunk_alloc(&global_mm_pool, acquire_order);
  if (page_addr != NULL) {
    log_detail(LOG_INFO, "allocate addr: %#x",
               chunk2virt(&global_mm_pool, page_addr));
    return chunk2virt(&global_mm_pool, page_addr);
  }
  warn("fail to alloc addr");
  return NULL;
}

static void kfree(void* ptr) {
  struct chunk* chunk = virt2chunk(&global_mm_pool, ptr);
  if (chunk && chunk->slab){
    printf("small:free in slab\n");
		free_in_slab(ptr);
  }
  else{
    printf("big:free in buddy system\n");
    chunk_free(&global_mm_pool, chunk);
    info_detail("free successfully, address: %#x", ptr);
  }
}

static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);

  void* pg_start = NULL;
  void* pi_start = NULL;
  int nr_page;
  nr_page =
      (uint64_t)(heap.end - heap.start) / (SZ_PAGE + sizeof(struct chunk));
  pg_start = heap.start;
  pi_start = (bool*)(pg_start + nr_page * SZ_PAGE);
  // global_mm_pool = pi_start;
  // pi_start       = (struct pmm_pool*)pi_start + 1;
  log_detail(
      LOG_INFO,
      "page start addr: %#x, page indicator addr: %#x, available page: %d",
      pg_start, pi_start, nr_page);
  log_detail(LOG_INFO, "begin to init buddy system");
  buddy_init(&global_mm_pool, pi_start, pg_start, nr_page);
  printf("begin to init slabbbbb\n");
  slab_init();

  printf("begin to test\n");
  printf("test slab\n");
  kfree(kalloc(_SIZE-1));
  printf("test buddy system\n");
  kfree(kalloc(SZ_PAGE*4));
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
