#include <devices.h>
#include <fs.h>
#include <file.h>

#define D "sda"
superblock_t sb;
spinlock_t fslock = {.lock_flag = false, .name = "fslock", .hold_cpuid = -1};
inode_t inodes[NBLOCK];

// Read block
void fs_readblk(device_t* dev, uint32_t blk_no, block_t* buf) {
  buf->blk_no = blk_no;
  dev->ops->read(dev, OFFSET_BLOCK(blk_no), buf->data, BSIZE);
}

// Write block
void fs_writeblk(device_t* dev, uint32_t blk_no, block_t* buf) {
  dev->ops->write(dev, OFFSET_BLOCK(blk_no), buf->data, BSIZE);
}

// set whole block to zero
void fs_zeroblk(device_t* dev, uint32_t blk_no) {
  block_t zero = {
      .blk_no = blk_no,
  };
  memset(zero.data, 0, sizeof(BSIZE));
  dev->ops->write(dev, OFFSET_BLOCK(blk_no), zero.data, BSIZE);
}

// alloc a free block(bit map 0),return block number
uint32_t fs_allocblk(device_t* dev) {
  uint32_t i, j, k;
  block_t block;
  for (i = 0; i < NBLOCK; i += BSIZE * 8) {
    block.blk_no = ROUNDUP_BLK_NUM((OFFSET_BITMAP(i)))/ BSIZE;
    // read bit map block
    dev->ops->read(dev, OFFSET_BITMAP(i), block.data, BSIZE);
    for (j = 0; j < BSIZE * 8 && i + j <= NBLOCK; j++) {
      // k is the byte that contains bitmap number j
      k = j / 8;
      // bit is 0 ,block is free
      if (!(block.data[k] & (1 << (7 - (j % 8))))) {
        // set bit to be used
        block.data[k] |= (1 << (7 - (j % 8)));
        dev->ops->write(dev, OFFSET_BITMAP(i + j), &(block.data[k]), 1);
        uint32_t blk_no = i + j;
        fs_zeroblk(dev, blk_no);
        return blk_no;
      }
    }
  }
  panic("[fs.c/fs_allocblk]:no free block");
}

// free an alloced block
void fs_freeblk(device_t* dev, uint32_t blk_no) {
  uint8_t byte;
  dev->ops->read(dev, OFFSET_BITMAP(blk_no), &byte, 1);
  byte = byte & ~(1 << (7 - (blk_no % 8)));
  dev->ops->write(dev, OFFSET_BITMAP(blk_no), &byte, 1);
}

// read superblock
void fs_readsb(device_t* dev, superblock_t* sb) {
  block_t block;
  dev->ops->read(dev, OFFSET_BOOT, block.data, BSIZE);
  memmove(sb, block.data, sizeof(*sb));
}

// create superblock
void fs_createsb(device_t* dev, superblock_t* sb) {
  dev->ops->write(dev, OFFSET_BOOT, sb, sizeof(*sb));
}

// read an inode
void fs_readinode(device_t* dev, uint32_t inode_no, inode_t* inode) {
  inode->dev  = dev;
  inode->inum = inode_no;
  inode->ref  = 0;
  spin_init(&(inode->lock), "");
  dev->ops->read(dev, OFFSET_INODE(inode_no), (&(inode->type)),
                 sizeof(dinode_t));
}

// write an inode
void fs_writeinode(device_t* dev, uint32_t inode_no, inode_t* inode) {
  // dinode_t* di = (dinode_t*)(&(inode->type));
  // printf("\nwrite dinode is "
  //        ":\n\tpos:%d\n\ttype:%d\n\tsize:%d\n\tnlinks:\n\taddrs:",
  //        OFFSET_INODE(inode_no), di->type, di->size, di->nlink);
  // for (int i = 0; i <= NDIRECT; i++) {
  //   printf("[%d] ", di->addrs[i]);
  // }
  // printf("\n");
  dev->ops->write(dev, OFFSET_INODE(inode_no), (dinode_t*)(&(inode->type)),
                  sizeof(dinode_t));
  // dev->ops->write(dev, OFFSET_INODE(inode_no), di, sizeof(dinode_t));
}

// ok
void fs_initinodes(device_t* dev) {
  for (int i = 0; i < NBLOCK; i++) {
    inodes[i].dev  = dev;
    inodes[i].inum = i;
    fs_writeinode(dev, i, &inodes[i]);
  }
}

// ok
void fs_initblks(device_t* dev) {
  for (int i = 0; i < NBLOCK; i++) {
    fs_zeroblk(dev, i);
    fs_freeblk(dev, i);
  }
}

// initial superblock
void fs_init() {
  sb.nblocks    = NBLOCK;
  sb.ninodes    = NBLOCK;
  sb.inodestart = OFFSET_INODE(0) / BSIZE;
  sb.bmapstart  = OFFSET_BITMAP(0) / BSIZE;
  sb.size       = OFFSET_BLOCK(NBLOCK - 1);
  fs_createsb(dev->lookup(D), &sb);
  fs_initinodes(dev->lookup(D));
  fs_initblks(dev->lookup(D));
  fs_allocblk(dev->lookup(D));
  file_init();
}


// inode operations

inode_t* iget(uint32_t inum) {
  spin_lock(&inodes[inum].lock);
  inodes[inum].ref++;
  spin_unlock(&inodes[inum].lock);
  return &inodes[inum];
}

// ok
inode_t* ialloc(short type) {
  int i;
  for (i = 0; i < NBLOCK; i++) {
    if (inodes[i].type == 0) {
      inodes[i].type = type;
      fs_writeinode(dev->lookup("sda"), i, &inodes[i]);
      return iget(i);
    }
  }
  panic("ialloc: no inodes");
}

// ok
void iupdate(inode_t* ip) {
  fs_writeinode(dev->lookup("sda"), ip->inum, ip);
}

// ok
inode_t* idup(inode_t* ip) {
  spin_lock(&ip->lock);
  ip->ref++;
  spin_unlock(&ip->lock);
  return ip;
}

// ok
void ilock(inode_t* ip) {
  if (ip == 0 || ip->ref < 1) panic("ilock");

  spin_lock(&ip->lock);
}

// ok
void iunlock(inode_t* ip) {
  if (ip == 0 || !spin_holding(&ip->lock) || ip->ref < 1) panic("iunlock");

  spin_unlock(&ip->lock);
}

void itrunc(inode_t* ip) {
  int i, j;
  block_t buf;
  uint32_t* addr;

  for (i = 0; i < NDIRECT; i++) {
    if (ip->addrs[i]) {
      fs_zeroblk(ip->dev, ip->addrs[i]);
      fs_freeblk(ip->dev, ip->addrs[i]);
      ip->addrs[i] = 0;
    }
  }

  if (ip->addrs[NDIRECT]) {
    fs_readblk(ip->dev, ip->addrs[NDIRECT], &buf);
    addr = (uint32_t*)buf.data;
    for (j = 0; j < NINDIRECT; j++) {
      if (addr) {
        fs_zeroblk(ip->dev, *addr);
        fs_freeblk(ip->dev, *addr);
      }
      addr++;
    }
    fs_zeroblk(ip->dev, ip->addrs[NDIRECT]);
    fs_freeblk(ip->dev, ip->addrs[NDIRECT]);
    ip->addrs[NDIRECT] = 0;
  }
  ip->size = 0;
  iupdate(ip);
}

void iput(inode_t* ip) {
  spin_lock(&ip->lock);
  if (ip->type != 0 && ip->nlink == 0) {
    int r = ip->ref;
    if (r == 1) {
      // inode has no links and no other references: truncate and free.
      itrunc(ip);
      ip->type = 0;
      iupdate(ip);
    }
  }
  if (ip->ref != 0) {
    ip->ref--;
  }
  spin_unlock(&ip->lock);
}

void iunlockput(inode_t* ip) {
  iunlock(ip);
  iput(ip);
}

// ok
int readi(inode_t* ip, char* dst, uint32_t off, uint32_t n) {
  if (off > ip->size || off + n < off) return -1;
  if (off + n > ip->size) n = ip->size - off;

  int i;
  char* offnow;
  int startblk = off / BSIZE, offstart = off % BSIZE,
      endblk = (off + n) / BSIZE, offend = (off + n) % BSIZE;
  // printf("off:%d\nn:%d\nstartblk:%d\noffstart:%d\nendblk:%d\noffend:%d\n", off,
  //        n, startblk, offstart, endblk, offend);
  block_t buf;
  if (startblk < NDIRECT && endblk < NDIRECT) {
    // printf("-----1-----\n");
    if (startblk == endblk) {
      fs_readblk(ip->dev, ip->addrs[startblk], &buf);
      memmove(dst, buf.data + offstart, n);
    } else {
      fs_readblk(ip->dev, ip->addrs[startblk], &buf);
      offnow = dst;
      memmove(offnow, buf.data + offstart, BSIZE - offstart);
      offnow += (BSIZE - offstart);
      for (i = startblk + 1; i < endblk; i++) {
        fs_readblk(ip->dev, ip->addrs[i], &buf);
        memmove(offnow, buf.data, BSIZE);
        offnow += BSIZE;
      }
      fs_readblk(ip->dev, ip->addrs[endblk], &buf);
      memmove(offnow, buf.data, offend);
      offnow += offend;
    }
  } else if (startblk < NDIRECT && endblk >= NDIRECT) {
    // printf("-----2-----\n");
    fs_readblk(ip->dev, ip->addrs[startblk], &buf);
    offnow = dst;
    memmove(offnow, buf.data + offstart, BSIZE - offstart);
    offnow += (BSIZE - offstart);
    for (i = startblk + 1; i < NDIRECT; i++) {
      fs_readblk(ip->dev, ip->addrs[i], &buf);
      memmove(offnow, buf.data, BSIZE);
      offnow += BSIZE;
    }
    block_t tmp;
    fs_readblk(ip->dev, ip->addrs[NDIRECT], &tmp);
    uint32_t* addr = (uint32_t*)tmp.data;
    for (i = NDIRECT; i < endblk; i++) {
      fs_readblk(ip->dev, *addr, &buf);
      memmove(offnow, buf.data, BSIZE);
      offnow += BSIZE;
      addr++;
    }
    fs_readblk(ip->dev, *addr, &buf);
    memmove(offnow, buf.data, offend);
    offnow += offend;
    addr++;
  } else if (startblk >= NDIRECT && endblk >= NDIRECT) {
    // printf("-----3-----\n");
    block_t tmp;
    fs_readblk(ip->dev, ip->addrs[NDIRECT], &tmp);
    uint32_t* addr = (uint32_t*)tmp.data;
    addr += startblk - 12;
    if (startblk == endblk) {
      fs_readblk(ip->dev, *addr, &buf);
      memmove(dst, buf.data + offstart, n);
    } else {
      fs_readblk(ip->dev, *addr, &buf);
      offnow = dst;
      memmove(offnow, buf.data + offstart, BSIZE - offstart);
      offnow += (BSIZE - offstart);
      addr++;
      for (i = startblk + 1; i < endblk; i++) {
        fs_readblk(ip->dev, *addr, &buf);
        memmove(offnow, buf.data, BSIZE);
        offnow += BSIZE;
        addr++;
      }
      fs_readblk(ip->dev, *addr, &buf);
      memmove(offnow, buf.data, offend);
      offnow += offend;
      addr++;
    }
  } else {
    panic("readi error\n");
  }

  return n;
}

// ok
int writei(inode_t* ip, char* src, uint32_t off, uint32_t n) {
  if (off > ip->size || off + n < off) return -1;
  if (off + n > MAX_FILE_SIZE) return -1;

  int i;
  char* offnow;
  int startblk = off / BSIZE, offstart = off % BSIZE,
      endblk = (off + n) / BSIZE, offend = (off + n) % BSIZE;
  // printf("off:%d\nn:%d\nstartblk:%d\noffstart:%d\nendblk:%d\noffend:%d\n",off,n,startblk,offstart,endblk,offend);
  block_t buf;
  if (startblk < NDIRECT && endblk < NDIRECT) {
    // printf("-----1-----\n");
    if (startblk == endblk) {
      if (ip->addrs[startblk] == 0) {
        ip->addrs[startblk] = fs_allocblk(ip->dev);
      }
      fs_readblk(ip->dev, ip->addrs[startblk], &buf);
      memmove(buf.data + offstart, src, n);
      fs_writeblk(ip->dev, ip->addrs[startblk], &buf);
    } else {
      if (ip->addrs[startblk] == 0) {
        ip->addrs[startblk] = fs_allocblk(ip->dev);
      }
      fs_readblk(ip->dev, ip->addrs[startblk], &buf);
      offnow = src;
      memmove(buf.data + offstart, offnow, BSIZE - offstart);
      fs_writeblk(ip->dev, ip->addrs[startblk], &buf);
      offnow += (BSIZE - offstart);
      for (i = startblk + 1; i < endblk; i++) {
        if (ip->addrs[i] == 0) {
          ip->addrs[i] = fs_allocblk(ip->dev);
        }
        fs_readblk(ip->dev, ip->addrs[i], &buf);
        memmove(buf.data, offnow, BSIZE);
        fs_writeblk(ip->dev, ip->addrs[i], &buf);
        offnow += BSIZE;
      }
      if (ip->addrs[endblk] == 0) {
        ip->addrs[endblk] = fs_allocblk(ip->dev);
      }
      fs_readblk(ip->dev, ip->addrs[endblk], &buf);
      memmove(buf.data, offnow, offend);
      fs_writeblk(ip->dev, ip->addrs[endblk], &buf);
      offnow += offend;
    }
  } else if (startblk < NDIRECT && endblk >= NDIRECT) {
    // printf("-----2-----\n");
    if (ip->addrs[startblk] == 0) {
      ip->addrs[startblk] = fs_allocblk(ip->dev);
    }
    fs_readblk(ip->dev, ip->addrs[startblk], &buf);
    offnow = src;
    memmove(buf.data + offstart, offnow, BSIZE - offstart);
    fs_writeblk(ip->dev, ip->addrs[startblk], &buf);
    offnow += (BSIZE - offstart);
    for (i = startblk + 1; i < NDIRECT; i++) {
      if (ip->addrs[i] == 0) {
        ip->addrs[i] = fs_allocblk(ip->dev);
      }
      fs_readblk(ip->dev, ip->addrs[i], &buf);
      memmove(buf.data, offnow, BSIZE);
      fs_writeblk(ip->dev, ip->addrs[i], &buf);
      offnow += BSIZE;
    }
    block_t tmp;
    if (ip->addrs[NDIRECT] == 0) {
      ip->addrs[NDIRECT] = fs_allocblk(ip->dev);
    }
    fs_readblk(ip->dev, ip->addrs[NDIRECT], &tmp);
    uint32_t* addr = (uint32_t*)tmp.data;
    for (i = NDIRECT; i < endblk; i++) {
      if (*addr == 0) {
        *addr = fs_allocblk(ip->dev);
      }
      fs_readblk(ip->dev, *addr, &buf);
      memmove(buf.data, offnow, BSIZE);
      fs_readblk(ip->dev, *addr, &buf);
      offnow += BSIZE;
      addr++;
    }
    if (*addr == 0) {
      *addr = fs_allocblk(ip->dev);
    }
    fs_readblk(ip->dev, *addr, &buf);
    memmove(buf.data, offnow, offend);
    fs_writeblk(ip->dev, *addr, &buf);
    offnow += offend;
    addr++;
    fs_writeblk(ip->dev, ip->addrs[NDIRECT], &tmp);
  } else if (startblk >= NDIRECT && endblk >= NDIRECT) {
    //printf("-----3-----\n");
    block_t tmp;
    if (ip->addrs[NDIRECT] == 0) {
      ip->addrs[NDIRECT] = fs_allocblk(ip->dev);
    }
    fs_readblk(ip->dev, ip->addrs[NDIRECT], &tmp);
    uint32_t* addr = (uint32_t*)tmp.data;
    addr += startblk - 12;
    if (startblk == endblk) {
      if (*addr == 0) {
        *addr = fs_allocblk(ip->dev);
      }
      fs_readblk(ip->dev, *addr, &buf);
      memmove(buf.data + offstart, src, n);
      fs_writeblk(ip->dev, *addr, &buf);
    } else {
      if (*addr == 0) {
        *addr = fs_allocblk(ip->dev);
      }
      fs_readblk(ip->dev, *addr, &buf);
      offnow = src;
      memmove(buf.data + offstart, offnow, BSIZE - offstart);
      fs_writeblk(ip->dev, *addr, &buf);
      offnow += (BSIZE - offstart);
      addr++;
      for (i = startblk + 1; i < endblk; i++) {
        if (*addr == 0) {
          *addr = fs_allocblk(ip->dev);
        }
        fs_readblk(ip->dev, *addr, &buf);
        memmove(buf.data, offnow, BSIZE);
        fs_writeblk(ip->dev, *addr, &buf);
        offnow += BSIZE;
        addr++;
      }
      if (*addr == 0) {
        *addr = fs_allocblk(ip->dev);
      }
      fs_readblk(ip->dev, *addr, &buf);
      memmove(buf.data, offnow, offend);
      fs_writeblk(ip->dev, *addr, &buf);
      offnow += offend;
      addr++;
    }
    fs_writeblk(ip->dev, ip->addrs[NDIRECT], &tmp);
  } else {
    panic("readi error\n");
  }

  if (n > 0 && off + n > ip->size) {
    ip->size = off + n;
    iupdate(ip);
  }

  //printf("size:%d\n", ip->size);
  return n;
}

void inode_print(inode_t* ip) {
  printf("\ninode is "
         ":\n\tinum:%d\n\ttype:%d\n\tsize:%d\n\tnlinks:%d\n\tref:%d\n\taddrs:",
         ip->inum, ip->type, ip->size, ip->nlink, ip->ref);
  for (int i = 0; i <= NDIRECT; i++) {
    printf("[%d] ", ip->addrs[i]);
  }
  printf("\n");
}

MODULE_DEF(fs) = {
    .init       = fs_init,
    .readblk    = fs_readblk,
    .writeblk   = fs_writeblk,
    .zeroblk    = fs_zeroblk,
    .allocblk   = fs_allocblk,
    .freeblk    = fs_freeblk,
    .readinode  = fs_readinode,
    .writeinode = fs_writeinode,
};