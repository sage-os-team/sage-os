#include <kernel.h>
#include <klib.h>
#include <am.h>
#include <logger.h>
#include <file.h>
#include <fs.h>
#include "trap.h"

task_t* task;
extern inode_t inodes[NBLOCK];

void part(){
  printf("-----------------------------------------------------------------------------------------------\n");
}

void func(void* arg) {

  //------------------------test  offset------------------------------------
  int o1, o2, o3, o4, o5, o6;
  o1 = OFFSET_BOOT;
  o6 = OFFSET_SB;
  o2 = OFFSET_ALLINODE;
  o3 = OFFSET_ALLBITMAP;
  o4 = OFFSET_BLOCK(0);
  o5 = OFFSET_BLOCK((NBLOCK));
  // printf("OFFSET_BOOT is %d\n", o1);
  // printf("OFFSET_ALLINODE is %d\n", o2);
  // printf("OFFSET_ALLBITMAP is %d\n", o3);
  // printf("OFFSET_BLOCK 0 is %d\n", o4);
  // printf("OFFSET_BLOCK N is %d\n", o5);

  printf("------------------------\n");
  printf("|\tBOOT      \t|\n");
  printf("|   \t(%d)blocks\t|\n", o1 / BSIZE);
  printf("|end at (0x%8x)\t|\n", o1);
  printf("------------------------\n");
  printf("|\tSUPER BLOCK\t|\n");
  printf("|   \t(%d)blocks\t|\n", (o6) / BSIZE);
  printf("|end at (0x%8x)\t|\n", o1 + o6);
  printf("------------------------\n");
  printf("|\tINODE      \t|\n");
  printf("|   \t(%d)blocks\t|\n", (o2 - o6 - o1) / BSIZE);
  printf("|end at (0x%8x)\t|\n", o2);
  printf("------------------------\n");
  printf("|\tBIT MAP    \t|\n");
  printf("|   \t(%d)blocks\t|\n", (ROUNDUP_BLK_NUM(o3 - o2)) / BSIZE);
  printf("|end at (0x%8x)\t|\n", o3);
  printf("------------------------\n");
  printf("|\tDATA BLOCK\t|\n");
  printf("|   \t(%d)blocks\t|\n", (o5 - o4) / BSIZE);
  printf("|end at (0x%8x)\t|\n", o5);
  printf("------------------------\n");

  // //------------------------test data block rw------------------------------
  uint64_t start=safe_io_read(AM_TIMER_UPTIME).us;
  block_t buf;
  for (int i = 1; i <= 20; i++){
    sprintf((char*)buf.data,"this is data block %d.",i % 2 ? i : i + 100);
    fs->writeblk(dev->lookup("sda"), i % 2 ? i : i + 100, &buf);
  }
  block_t out;
  for (int i = 1; i <= 20; i++) {
    fs->readblk(dev->lookup("sda"), i % 2 ? i : i + 100, &out);
    printf("data %d is :[%s]\n", i, out.data);
  }
  uint64_t end=safe_io_read(AM_TIMER_UPTIME).us;
  printf("random RW 100 blocks time: %d ms\n",(end-start)/1000);

  //------------------------test inoderw-----------------------------------
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
      "\ninode is:\n\tnum: %d\n\ttype: %d\n\tsize: %d\n\tnlinks:%d\n\taddrs:",
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

  fs_print_datablock_bitmap_info(0);
  fs_print_inode_info(0);

  part();

  printf("test inode:\n");
  inode_t* node=&inodes[10];
  inode_print(node->inum);
  iget(node->inum);
  printf("after iget:");
  inode_print(node->inum);

  part();

  printf("test ialloc:\n");
  inode_t* node1=ialloc(DINODE_TYPE_F);
  inode_t* node2=ialloc(DINODE_TYPE_F);
  inode_print(node1->inum);
  inode_print(node2->inum);

  part();

  printf("test idup:\n");
  inode_print(node->inum);
  node=idup(node);
  inode_print(node->inum);

  part();
  printf("test readi and writei:\n");
  inode_t* writein=ialloc(DINODE_TYPE_F);
  inode_print(writein->inum);
  block_t t;
  for(int i=0;i<BSIZE;i++){
    t.data[i]='a';
  }
  for(int i=0;i<NDIRECT;i++){
    writei(writein,(char*)t.data,i*BSIZE,BSIZE);
    inode_print(writein->inum);
  }
  for(int i=0;i<BSIZE;i++){
    t.data[i]='b';
  }
  writei(writein,(char*)t.data,12*BSIZE-1,BSIZE);
  inode_print(writein->inum);
  for(int i=0;i<BSIZE;i++){
    t.data[i]='c';
  }
  writei(writein,(char*)t.data,13*BSIZE-1,BSIZE);
  inode_print(writein->inum);

  block_t tt;int rlen=0;
  rlen=readi(writein,(char*)tt.data,13*BSIZE-2,4);
  printf("read len:%d\n",rlen);
  printf("read result:%s\n",tt.data);
  rlen=readi(writein,(char*)tt.data,12*BSIZE-2,4);
  printf("read len:%d\n",rlen);
  printf("read result:%s\n",tt.data);

  part();

  printf("test itrunc\n");
  node=writein;
  fs_readblk(node->dev,node->addrs[NDIRECT],&tt);
  uint32_t* addr=(uint32_t*)tt.data;
  uint32_t addr1=*addr;
  printf("blk num:%d\n",addr1);
  fs_readblk(node->dev,addr1,&tt);
  printf("data:%s\n",tt.data);
  idup(node);
  inode_print(node->inum);
  iput(node);
  inode_print(node->inum);
  iput(node);
  inode_print(node->inum);
  fs_readblk(node->dev,addr1,&tt);
  printf("data:%s\n",tt.data);

  part();

  printf("test dir and path\n");
  char name1[10]="subdir1",name3[10]="subdir3",name2[10]="subdir2",name4[10]="subdir4",name5[10]="subdir5";
  // subdir1.inum=20;strncpy(subdir1.name,name1,10);
  // subdir2.inum=30;strncpy(subdir2.name,name2,10);
  // subdir3.inum=40;strncpy(subdir3.name,name3,10);
  // subdir4.inum=50;strncpy(subdir4.name,name4,10);
   inode_t* root=&inodes[1];root->type=DINODE_TYPE_D;
  // inode_t subdir1=inodes[20];
  // inode_t subdir2=inodes[30];
   inode_t* subdir3=&inodes[40];subdir3->type=DINODE_TYPE_D;
   inode_t* subdir4=&inodes[50];subdir4->type=DINODE_TYPE_D;

  dirlink(root,name1,20);
  dirlink(root,name2,30);
  dirlink(root,name3,40);
  dirlink(subdir3,name4,50);
  dirlink(subdir4,name5,60);

  inode_t* find1=dirlookup(root,name1,0);
  printf("%d+",find1->inum);
  inode_t* find2=dirlookup(root,name2,0);
  printf("%d+",find2->inum);
  inode_t* find3=dirlookup(root,name3,0);
  printf("%d+",find3->inum);
  inode_t* find4=dirlookup(subdir3,name4,0);
  printf("%d\n",find4->inum);

  char path[50]="/subdir3/subdir4/subdir5";
  inode_t* res=namei(path);
  printf("\ntest namei:%d\n",res->inum);
  
  part();

  while (1);
  
  //panic("test end");
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