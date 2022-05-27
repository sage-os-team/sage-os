#include <kernel.h>
#include <klib.h>
#include <am.h>
#include <logger.h>
#include <file.h>
#include <fs.h>
#include "trap.h"

task_t* task;

void func(void* arg) {
  char* buf      = "this is a test file data.";
  char* out      = "";
  inode_t* inode = ialloc(DINODE_TYPE_F);
  // fs->readinode(dev->lookup("sda"), inode.inum, &inodeout);
  // printf(
  //     "\ninode is:\n\tnum: %d\n\ttype: %d\n\tsize: %d\n\tnlinks: %d\n\taddrs:
  //     ", inodeout.inum, inodeout.type, inodeout.size, inodeout.nlink);
  // for (int i = 0; i <= NDIRECT; i++) {
  //   printf("[%d] ", inodeout.addrs[i]);
  // }
  // printf("\n");
  int fd      = file_alloc();
  file_t* f   = file_get(fd);
  f->iptr     = inode;
  f->writable = 1;
  f->readable = 1;
  file_write(f, buf, 25);
  f->off = 0;
  file_read(f, out, 25);
  int fd2    = file_dup(f);
  stat_t* st = pmm->alloc(sizeof(stat_t));
  file_stat(f, st);
  printf("%d", fd2);
  file_close(f);
}

int main() {
  _log_mask = LOG_ERROR | LOG_INFO;
  ioe_init();
  cte_init(os->trap);
  os->init();
  vme_init(pmm->pgalloc, pmm->free);

  fs->init();

  task = pmm->alloc(sizeof(task_t));
  kmt->create(task, "file", func, NULL);

  mpe_init(os->run);

  while (1)
    ;
  return 1;
}