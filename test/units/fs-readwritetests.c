#include <kernel.h>
#include <klib.h>
#include <am.h>
#include <logger.h>
#include <file.h>
#include <fs.h>
#include <io.h>

#define MAX_COUNT 100
#define MODE      1

task_t *task_reader, *task_writer;
spinlock_t print_lock;
spinlock_t cnt_lock;
int cnt = 0, read = 1, write = 1;

void writer(void *arg) {
  block_t blkw;
  uint64_t time = 0;
  while (1) {
    spin_lock(&cnt_lock);
    if (write > MAX_COUNT) {
      spin_unlock(&cnt_lock);
      break;
    }
    cnt++;
    if (MODE) {
      blkw.blk_no = write % 2 ? write++ : write++ + 100;
    } else {
      blkw.blk_no = write++;
    }

    sprintf((char *)blkw.data, "this is data block %d.\000", blkw.blk_no);
    uint64_t s, e;
    s = safe_io_read(AM_TIMER_UPTIME).us;
    fs->writeblk(dev->lookup("sda"), blkw.blk_no, &blkw);
    e = safe_io_read(AM_TIMER_UPTIME).us;
    time += e - s;
    spin_lock(&print_lock);
    printf("W %d:%s\n", blkw.blk_no, blkw.data);
    spin_unlock(&print_lock);
    spin_unlock(&cnt_lock);
  }
  info("Write %d blocks,times: %d ms", MAX_COUNT, time / 1000);
  while (1)
    ;
}

void reader(void *arg) {
  block_t blkr;
  uint64_t time = 0;
  while (1) {
    spin_lock(&cnt_lock);
    if (read > MAX_COUNT) {
      spin_unlock(&cnt_lock);
      break;
    }
    if (cnt > 0) {
      cnt--;
      if (MODE) {
        blkr.blk_no = read % 2 ? read++ : read++ + 100;
      } else {
        blkr.blk_no = read++;
      }
      uint64_t s, e;
      s = safe_io_read(AM_TIMER_UPTIME).us;
      fs->readblk(dev->lookup("sda"), blkr.blk_no, &blkr);
      e = safe_io_read(AM_TIMER_UPTIME).us;
      time += e - s;
      spin_lock(&print_lock);
      printf("\tR %d:%s\n", blkr.blk_no, blkr.data);
      spin_unlock(&print_lock);
      spin_unlock(&cnt_lock);

    } else {
      spin_unlock(&cnt_lock);
    }
  }
  info("Read %d blocks,times: %d ms", MAX_COUNT, time / 1000);

  while (1)
    ;
}

static void create_threads() {
  task_reader = pmm->alloc(sizeof(task_t));
  task_writer = pmm->alloc(sizeof(task_t));
  kmt->create(task_writer, "writer", writer, NULL);
  kmt->create(task_reader, "reader", reader, NULL);
}

int main() {
  _log_mask = LOG_ERROR | LOG_INFO;
  ioe_init();
  cte_init(os->trap);
  os->init();
  vme_init(pmm->pgalloc, pmm->free);

  fs->init();
  spin_init(&print_lock, "print_lock");
  spin_init(&cnt_lock, "cnt_lock");

  create_threads();

  mpe_init(os->run);

  while (1)
    ;
  return 1;
}