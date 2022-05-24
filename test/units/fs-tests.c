#include <kernel.h>
#include <klib.h>
#include <am.h>
#include <logger.h>
#include <file.h>
#include <fs.h>

int main() {
  _log_mask = LOG_ERROR | LOG_INFO;
  ioe_init();
  cte_init(os->trap);
  os->init();
  vme_init(pmm->pgalloc, pmm->free);

  fs->init();

  //------------------------test  offset------------------------------------
  int o1, o2, o3, o4, o5;
  o1 = OFFSET_BOOT;
  o2 = OFFSET_ALLINODE;
  o3 = OFFSET_ALLBITMAP;
  o4 = OFFSET_BLOCK(0);
  o5 = OFFSET_BLOCK((NBLOCK - 1));
  printf("OFFSET_BOOT is %d\n", o1);
  printf("OFFSET_ALLINODE is %d\n", o2);
  printf("OFFSET_ALLBITMAP is %d\n", o3);
  printf("OFFSET_BLOCK 0 is %d\n", o4);
  printf("OFFSET_BLOCK N is %d\n", o5);

  //------------------------test data block rw------------------------------
  block_t buf = {.blk_no = 0, .data = "this is a test block data"};
  fs->writeblk(dev->lookup("sda"), 0, &buf);
  block_t out;
  fs->readblk(dev->lookup("sda"), 0, &out);
  printf("data is :[%s]\n", out.data);

  //------------------------test inode rw-----------------------------------
  inode_t inode, inodeout;
  inode.dev   = dev->lookup("sda");
  inode.size  = 256;
  inode.inum  = 2;
  inode.nlink = 5;
  inode.type  = DINODE_TYPE_D;
  for (int i = 0; i < NDIRECT; i++) {
    inode.addrs[i] = OFFSET_BLOCK(i);
  }
  fs->writeinode(dev->lookup("sda"), inode.inum, &inode);
  fs->readinode(dev->lookup("sda"), inode.inum, &inodeout);
  printf(
      "\ninode is:\n\tnum: %d\n\ttype: %d\n\tsize: %d\n\tnlinks: %d\n\taddrs: ",
      inodeout.inum, inodeout.type, inodeout.size, inodeout.nlink);
  for (int i = 0; i <= NDIRECT; i++) {
    printf("[%d] ", inodeout.addrs[i]);
  }
  printf("\n");

  //------------------------test block alloc/free---------------------------
  int free[5] = {1, 3, 6, 9, 16};
  printf("OFFSET BITMAP:");
  for (int i = 0; i < 5; i++) {
    printf("%d(%d) ", free[i], OFFSET_BITMAP(free[i]));
  }
  printf("\n");
  // alloc 20 block 0~19
  for (int i = 0; i < 20; i++) {
    uint32_t blk_no = fs->allocblk(dev->lookup("sda"));
    printf("alloc %d block is :%d\n", i + 1, blk_no);
  }
  // free block 1,3,6,9,16
  for (int i = 0; i < 5; i++) {
    fs->freeblk(dev->lookup("sda"), free[i]);
  }
  // alloc again
  for (int i = 0; i < 20; i++) {
    int blk_no = fs->allocblk(dev->lookup("sda"));
    printf("alloc %d block is :%d\n", i + 1, blk_no);
  }

  mpe_init(os->run);

  while (1)
    ;
  return 1;
}