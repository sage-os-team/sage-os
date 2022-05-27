#ifndef __FS_H__
#define __FS_H__

#include <common.h>
#include <sem.h>
#include <spinlock.h>

#include <fs_defs.h>

// inode operations
inode_t* iget(uint32_t inum);
inode_t* ialloc(short type);
void iupdate(inode_t* ip);
inode_t* idup(inode_t* ip);
void ilock(inode_t* ip);
void iunlock(inode_t* ip);
void itrunc(inode_t* ip);
void iput(inode_t* ip);
void iunlockput(inode_t* ip);
int readi(inode_t* ip, char* dst, uint32_t off, uint32_t n);
int writei(inode_t* ip, char* src, uint32_t off, uint32_t n);

int namecmp(const char* s, const char* t);
inode_t* dirlookup(inode_t* dp, char* name, uint32_t* poff);
int dirlink(inode_t* dp, char* name, uint32_t inum);

inode_t* namei(const char* path);
inode_t* nameiparent(const char* path, char* name);

void fs_print_datablock_bitmap_info(int level);
void fs_print_inode_info(int level);
void inode_print(int inum);
void fs_readblk(device_t* dev, uint32_t blk_no, block_t* buf);

#endif
