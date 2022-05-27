#include <user.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
//#include <dirent.h>

#include "../kernel/framework/fs_defs.h"

const char img_path[] = "fs-img";

uint32_t allocblk(uint8_t *disk);
void readblk(uint8_t *disk, uint32_t blk_no, block_t *buf);
void writeblk(uint8_t *disk, uint32_t blk_no, block_t *buf);
void zeroblk(uint8_t *disk, uint32_t blk_no);
void writeinode(uint8_t *disk, uint32_t inode_no, dinode_t *inode);
int writei(uint8_t *disk, dinode_t *ip, char *src, uint32_t off, uint32_t n);
void writeimg(dinode_t parent, int *inum, const char *path, int type);

int main(int argc, char *argv[]) {
  int fd, size = atoi(argv[1]) << 20;
  uint8_t *disk;

  // TODO: argument parsing

  assert((fd = open(argv[2], O_RDWR)) > 0);
  assert((ftruncate(fd, size)) == 0);
  assert((disk = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) !=
         (void *)-1);

  // TODO: mkfs
  dinode_t root;
  superblock_t sb;
  // write super block
  sb.nblocks    = NBLOCK;
  sb.ninodes    = NBLOCK;
  sb.inodestart = OFFSET_INODE(0) / BSIZE;
  sb.bmapstart  = OFFSET_BITMAP(0) / BSIZE;
  sb.size       = OFFSET_BLOCK(NBLOCK - 1);
  strncpy((char *)disk + OFFSET_BOOT, (char *)&sb, sizeof(superblock_t));

  // bit map set to 0
  char a = 0;
  strncpy((char *)disk + OFFSET_BITMAP(0), &a, NBLOCK / 8);

  // write root inode info
  allocblk(disk);
  root.type  = DINODE_TYPE_D;
  root.size  = 0;
  root.nlink = 1;
  memset(root.addrs, 0, sizeof(root.addrs));
  writeinode(disk, 1, &root);

  int i_no = 2;
  dirent_t dir;
  dinode_t inode;

  // .
  dir.inum = ROOTINO;
  strcpy(dir.name, ".\0");
  writei(disk, &root, (char *)&dir, 0, sizeof(dirent_t));

  // ..
  dir.inum = ROOTINO;
  strcpy(dir.name, "..\0");
  writei(disk, &root, (char *)&dir, sizeof(dirent_t), sizeof(dirent_t));
  // /a.txt
  // create dirent for a.txt
  dir.inum = i_no++;
  strcpy(dir.name, "a.txt\0");
  // create inode for a.txt
  inode.type  = DINODE_TYPE_F;
  inode.size  = 0;
  inode.nlink = 1;
  memset(inode.addrs, 0, sizeof(inode.addrs));
  // write dirent a.txt to root addrs
  writei(disk, &root, (char *)&dir, sizeof(dirent_t) * 2, sizeof(dirent_t));
  // write inode a.txt
  writeinode(disk, dir.inum, &inode);

  // /b
  dir.inum = i_no++;
  strcpy(dir.name, "b\0");

  inode.type  = DINODE_TYPE_D;
  inode.size  = 0;
  inode.nlink = 1;
  memset(inode.addrs, 0, sizeof(inode.addrs));

  writei(disk, &root, (char *)&dir, sizeof(dirent_t) * 3, sizeof(dirent_t));

  writeinode(disk, dir.inum, &inode);

  // /b/c.txt
  dir.inum = i_no++;
  strcpy(dir.name, "c.txt\0");
  writei(disk, &inode, (char *)&dir, sizeof(dirent_t), sizeof(dirent_t));
  inode.type  = DINODE_TYPE_F;
  inode.size  = 0;
  inode.nlink = 1;
  writeinode(disk, dir.inum, &inode);

  munmap(disk, size);
  close(fd);
}

// void writeimg(dinode_t parent, int *inum, const char *path, int type) {
//   DIR *dir   = opendir(path);
//   int nownum = inum;
//   int child  = 0;
//
//   if (dir != NULL) {
//     struct dirent *entry;
//     while ((entry = readdir(dir)) != NULL) {
//       if (strcmp(entry->d_name, ".") == 0) {
//       } else if (strcmp(entry->d_name, "..") == 0) {
//       }
//       dirent_t dirent;
//       dirent.inum = ++inum;
//       strcpy(dirent.name, entry->d_name);
//       dinode_t inode;
//       inode.type  = DINODE_TYPE_D;
//       inode.size  = 0;
//       inode.nlink = 0;
//       memset(inode.addrs, 0, sizeof(inode.addrs));
//     }
//   }
// }

uint32_t allocblk(uint8_t *disk) {
  uint32_t i, j, k;
  block_t block;
  for (i = 0; i < NBLOCK; i += BSIZE * 8) {
    block.blk_no = ROUNDUP_BLK_NUM((OFFSET_BITMAP(i))) / BSIZE;
    // read bit map block
    strncpy((char *)block.data, (char *)disk + OFFSET_BITMAP(i), BSIZE);
    for (j = 0; j < BSIZE * 8 && i + j <= NBLOCK; j++) {
      // k is the byte that contains bitmap number j
      k = j / 8;
      // bit is 0 ,block is free
      if (!(block.data[k] & (1 << (7 - (j % 8))))) {
        // set bit to be used
        block.data[k] |= (1 << (7 - (j % 8)));
        strncpy((char *)disk + OFFSET_BITMAP(i), (char *)&(block.data[k]), 1);
        uint32_t blk_no = i + j;
        zeroblk(disk, blk_no);
        return blk_no;
      }
    }
  }
  return -1;
}

// Read block
void readblk(uint8_t *disk, uint32_t blk_no, block_t *buf) {
  buf->blk_no = blk_no;
  strncpy((char *)buf->data, (char *)disk + OFFSET_BLOCK(blk_no), BSIZE);
}

// Write block
void writeblk(uint8_t *disk, uint32_t blk_no, block_t *buf) {
  strncpy((char *)disk + OFFSET_BLOCK(blk_no), (char *)buf->data, BSIZE);
}

void zeroblk(uint8_t *disk, uint32_t blk_no) {
  block_t zero = {
      .blk_no = blk_no,
  };
  memset(zero.data, 0, sizeof(BSIZE));
  strncpy((char *)disk + OFFSET_BLOCK(blk_no), (char *)(zero.data), BSIZE);
}

void writeinode(uint8_t *disk, uint32_t inode_no, dinode_t *inode) {
  strncpy((char *)disk + OFFSET_INODE(inode_no), (char *)(&(inode->type)),
          sizeof(dinode_t));
}

int writei(uint8_t *disk, dinode_t *ip, char *src, uint32_t off, uint32_t n) {
  if (off > ip->size || off + n < off) return -1;
  if (off + n > MAX_FILE_SIZE) return -1;

  int i;
  char *offnow;
  int startblk = off / BSIZE, offstart = off % BSIZE,
      endblk = (off + n) / BSIZE, offend = (off + n) % BSIZE;
  // printf("off:%d\nn:%d\nstartblk:%d\noffstart:%d\nendblk:%d\noffend:%d\n",off,n,startblk,offstart,endblk,offend);
  block_t buf;
  if (startblk < NDIRECT && endblk < NDIRECT) {
    // printf("-----1-----\n");
    if (startblk == endblk) {
      if (ip->addrs[startblk] == 0) {
        ip->addrs[startblk] = allocblk(disk);
      }
      readblk(disk, ip->addrs[startblk], &buf);
      memmove(buf.data + offstart, src, n);
      writeblk(disk, ip->addrs[startblk], &buf);
    } else {
      if (ip->addrs[startblk] == 0) {
        ip->addrs[startblk] = allocblk(disk);
      }
      readblk(disk, ip->addrs[startblk], &buf);
      offnow = src;
      memmove(buf.data + offstart, offnow, BSIZE - offstart);
      writeblk(disk, ip->addrs[startblk], &buf);
      offnow += (BSIZE - offstart);
      for (i = startblk + 1; i < endblk; i++) {
        if (ip->addrs[i] == 0) {
          ip->addrs[i] = allocblk(disk);
        }
        readblk(disk, ip->addrs[i], &buf);
        memmove(buf.data, offnow, BSIZE);
        writeblk(disk, ip->addrs[i], &buf);
        offnow += BSIZE;
      }
      if (ip->addrs[endblk] == 0) {
        ip->addrs[endblk] = allocblk(disk);
      }
      readblk(disk, ip->addrs[endblk], &buf);
      memmove(buf.data, offnow, offend);
      writeblk(disk, ip->addrs[endblk], &buf);
      offnow += offend;
    }
  } else if (startblk < NDIRECT && endblk >= NDIRECT) {
    // printf("-----2-----\n");
    if (ip->addrs[startblk] == 0) {
      ip->addrs[startblk] = allocblk(disk);
    }
    readblk(disk, ip->addrs[startblk], &buf);
    offnow = src;
    memmove(buf.data + offstart, offnow, BSIZE - offstart);
    writeblk(disk, ip->addrs[startblk], &buf);
    offnow += (BSIZE - offstart);
    for (i = startblk + 1; i < NDIRECT; i++) {
      if (ip->addrs[i] == 0) {
        ip->addrs[i] = allocblk(disk);
      }
      readblk(disk, ip->addrs[i], &buf);
      memmove(buf.data, offnow, BSIZE);
      writeblk(disk, ip->addrs[i], &buf);
      offnow += BSIZE;
    }
    block_t tmp;
    if (ip->addrs[NDIRECT] == 0) {
      ip->addrs[NDIRECT] = allocblk(disk);
    }
    readblk(disk, ip->addrs[NDIRECT], &tmp);
    uint32_t *addr = (uint32_t *)tmp.data;
    for (i = NDIRECT; i < endblk; i++) {
      if (*addr == 0) {
        *addr = allocblk(disk);
      }
      readblk(disk, *addr, &buf);
      memmove(buf.data, offnow, BSIZE);
      readblk(disk, *addr, &buf);
      offnow += BSIZE;
      addr++;
    }
    if (*addr == 0) {
      *addr = allocblk(disk);
    }
    readblk(disk, *addr, &buf);
    memmove(buf.data, offnow, offend);
    writeblk(disk, *addr, &buf);
    offnow += offend;
    addr++;
    writeblk(disk, ip->addrs[NDIRECT], &tmp);
  } else if (startblk >= NDIRECT && endblk >= NDIRECT) {
    // printf("-----3-----\n");
    block_t tmp;
    if (ip->addrs[NDIRECT] == 0) {
      ip->addrs[NDIRECT] = allocblk(disk);
    }
    readblk(disk, ip->addrs[NDIRECT], &tmp);
    uint32_t *addr = (uint32_t *)tmp.data;
    addr += startblk - 12;
    if (startblk == endblk) {
      if (*addr == 0) {
        *addr = allocblk(disk);
      }
      readblk(disk, *addr, &buf);
      memmove(buf.data + offstart, src, n);
      writeblk(disk, *addr, &buf);
    } else {
      if (*addr == 0) {
        *addr = allocblk(disk);
      }
      readblk(disk, *addr, &buf);
      offnow = src;
      memmove(buf.data + offstart, offnow, BSIZE - offstart);
      writeblk(disk, *addr, &buf);
      offnow += (BSIZE - offstart);
      addr++;
      for (i = startblk + 1; i < endblk; i++) {
        if (*addr == 0) {
          *addr = allocblk(disk);
        }
        readblk(disk, *addr, &buf);
        memmove(buf.data, offnow, BSIZE);
        writeblk(disk, *addr, &buf);
        offnow += BSIZE;
        addr++;
      }
      if (*addr == 0) {
        *addr = allocblk(disk);
      }
      readblk(disk, *addr, &buf);
      memmove(buf.data, offnow, offend);
      writeblk(disk, *addr, &buf);
      offnow += offend;
      addr++;
    }
    writeblk(disk, ip->addrs[NDIRECT], &tmp);
  } else {
  }

  if (n > 0 && off + n > ip->size) {
    ip->size = off + n;
  }

  // printf("size:%d\n", ip->size);
  return n;
}
