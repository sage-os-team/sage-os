/**
 * @file buddy.c
 * @author kg (kkwang@outlook.com)
 * @brief impl of buddy system
 * @version 0.1
 * @date 2022-03-16
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <buddy.h>
#include <common.h>
#include <logger.h>

/**
 * @brief initialize the buddy system
 *
 * @param mm_pool pointer of pmm-pool
 * @param start_chunk start address of the first chunk
 * @param start_addr start address of whole mm.
 * @param page_num the number of page to be managed.
 */
void buddy_init(struct pmm_pool* mm_pool, struct chunk* start_chunk,
                void* start_addr, uint64_t page_num) {
  struct chunk* chunk;
  int order;
  int chunk_idx;
  mm_pool->begin_addr     = (uintptr_t)start_addr;
  mm_pool->page_num       = page_num;
  mm_pool->size           = page_num * SZ_PAGE;
  mm_pool->chunk_metadata = start_chunk;
  for (order = 0; order < BUDDY_MAX_ORDER; order++) {
    // info("order:%d, addr: %#x", order, &mm_pool->free_lists[order]);
    mm_pool->free_lists[order].nr_free = 0;
    INIT_LIST_HEAD(&mm_pool->free_lists[order].free_list);
  }
  memset((char*)start_chunk, 0, page_num * sizeof(struct chunk));
  for (chunk_idx = 0; chunk_idx < page_num; chunk_idx++) {
    chunk        = start_chunk + chunk_idx;
    chunk->used  = 1;
    chunk->order = 0;
  }
  for (chunk_idx = 0; chunk_idx < page_num; chunk_idx++) {
    chunk = start_chunk + chunk_idx;
    chunk_free(mm_pool, chunk);
  }
}

/**
 * @brief free a chunk and process in deep.
 *
 * @param mm_pool pointer of pmm-pool
 * @param chunk
 */

void chunk_free(struct pmm_pool* mm_pool, struct chunk* chunk) {
  assert_msg(chunk->used, "try to FREE an unalloced page");
  chunk->used = false;
  chunk_append(mm_pool, chunk);
  chunk_merge(mm_pool, chunk);
}

/**
 * @brief make a chunk added to free list.
 *
 * @param mm_pool
 * @param chunk
 */
void chunk_append(struct pmm_pool* mm_pool, struct chunk* chunk) {
  struct free_list* free_list = &mm_pool->free_lists[chunk->order];
  list_add(&chunk->node, &free_list->free_list);
  free_list->nr_free++;
}

/**
 * @brief
 *
 * @param mm_pool
 * @param chunk
 * @return struct chunk*
 */
struct chunk* chunk_merge(struct pmm_pool* mm_pool, struct chunk* chunk) {
  if (chunk->used) {
    warn("try to MERGE an allocated page.");
    return NULL;
  }
  chunk_del(mm_pool, chunk);
  while (chunk->order < BUDDY_MAX_ORDER - 1) {
    struct chunk* buddy_chunk = get_buddy_chunk(mm_pool, chunk);

    if (buddy_chunk == NULL || buddy_chunk->used ||
        buddy_chunk->order != chunk->order) {
      break;
    }
    chunk_del(mm_pool, buddy_chunk);
    if (chunk > buddy_chunk) {
      struct chunk* tmp = chunk;
      chunk             = buddy_chunk;
      buddy_chunk       = tmp;
    }
    buddy_chunk->used = true;
    chunk->order++;
  }
  chunk_append(mm_pool, chunk);
  return chunk;
}

/**
 * @brief Get the buddy chunk
 *
 * @param mm_pool
 * @param chunk
 * @return struct chunk*
 */
struct chunk* get_buddy_chunk(struct pmm_pool* mm_pool, struct chunk* chunk) {
  intptr_t chunk_addr;
  intptr_t buddy_chunk_addr;
  int order;

  chunk_addr = (intptr_t)chunk2virt(mm_pool, chunk);
  order      = chunk->order;

#define BUDDY_PAGE_SIZE_ORDER (12)

  buddy_chunk_addr = chunk_addr ^ (1ul << (order + BUDDY_PAGE_SIZE_ORDER));

  if (buddy_chunk_addr < mm_pool->begin_addr ||
      buddy_chunk_addr >= mm_pool->begin_addr + mm_pool->size) {
    return NULL;
  }
  return virt2chunk(mm_pool, (void*)buddy_chunk_addr);
}

/**
 * @brief
 *
 * @param mm_pool
 * @param order
 * @return struct chunk*
 */
struct chunk* chunk_alloc(struct pmm_pool* mm_pool, uint8_t order) {
  int current_order = order;
  while (current_order < BUDDY_MAX_ORDER &&
         mm_pool->free_lists[current_order].nr_free <= 0)
    current_order++;
  if (current_order >= BUDDY_MAX_ORDER) {
    error("current order:%d try to ALLOCATE an buddy chunk greater than "
          "BUDDY_MAX_ORDER",
          current_order);
    return NULL;
  }
  struct chunk* chunk = list_entry(
      mm_pool->free_lists[current_order].free_list.next, struct chunk, node);
  if (chunk == NULL) {
    warn("get a NULL page");
    return NULL;
  }
  chunk_split(mm_pool, order, chunk);
  chunk->used = 1;
  return chunk;
}

/**
 * @brief
 *
 * @param mm_pool
 * @param chunk
 */
void chunk_del(struct pmm_pool* mm_pool, struct chunk* chunk) {
  struct free_list* free_list = &mm_pool->free_lists[chunk->order];
  list_del(&chunk->node);
  free_list->nr_free--;
}

/**
 * @brief
 *
 * @param mm_pool
 * @param order
 * @param chunk
 * @return struct chunk*
 */
struct chunk* chunk_split(struct pmm_pool* mm_pool, uint8_t order,
                          struct chunk* chunk) {
  if (chunk->used) {
    warn("try to SPLIT an allocated chunk");
    return NULL;
  }
  chunk->used = 0;
  chunk_del(mm_pool, chunk);

  while (chunk->order > order) {
    chunk->order--;
    struct chunk* buddy_chunk = get_buddy_chunk(mm_pool, chunk);
    if (buddy_chunk != NULL) {
      buddy_chunk->used  = 0;
      buddy_chunk->order = chunk->order;
      chunk_append(mm_pool, buddy_chunk);
    }
  }
  return chunk;
}

/**
 * @brief translate chunk address to virtual address
 *
 * @param mm_pool
 * @param chunk
 * @return void*
 */
void* chunk2virt(struct pmm_pool* mm_pool, struct chunk* chunk) {
  uintptr_t addr;
  addr = (chunk - mm_pool->chunk_metadata) * SZ_PAGE + mm_pool->begin_addr;
  return (void*)addr;
}

/**
 * @brief translate virtual address to chunk address
 *
 * @param mm_pool
 * @param virt
 * @return struct chunk*
 */
struct chunk* virt2chunk(struct pmm_pool* mm_pool, void* virt) {
  struct chunk* chunk;
  chunk = mm_pool->chunk_metadata +
          ((uintptr_t)virt - mm_pool->begin_addr) / SZ_PAGE;
  return chunk;
}
