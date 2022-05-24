#ifndef __FS_H__
#define __FS_H__

#include <common.h>
#include <sem.h>
#include <spinlock.h>

#define ROUNDUP_BLK_NUM(i) \
  (((i + BSIZE - 1) / BSIZE) * \
   BSIZE)  // offset of size (i) round up to block size's times
#define OFFSET_BOOT     (1024 * 1024)
#define OFFSET_SB       512
#define OFFSET_INODE(i) (OFFSET_BOOT + OFFSET_SB + (sizeof(dinode_t) * i))
#define OFFSET_ALLINODE \
  (OFFSET_BOOT + OFFSET_SB + ROUNDUP_BLK_NUM((sizeof(dinode_t) * NBLOCK)))
#define OFFSET_BITMAP(i) (OFFSET_ALLINODE + (i / 8))
#define OFFSET_ALLBITMAP (OFFSET_ALLINODE + NBLOCK / 8)
#define OFFSET_BLOCK(i)  (ROUNDUP_BLK_NUM(OFFSET_ALLBITMAP) + i * BSIZE)

#define NBLOCK        1024  // data block num
#define BSIZE         512   // block size
#define MAX_FILE_SIZE ((NDIRECT + NINDIRECT) * BSIZE)

#define DINODE_TYPE_N 0                           // unused
#define DINODE_TYPE_F 1                           // file
#define DINODE_TYPE_D 2                           // directory
#define PATH_LENGTH   32                          // path max length
#define NDIRECT       12                          // num of direct address
#define NINDIRECT     (BSIZE / sizeof(uint32_t))  // 128
#define MAXFILE       (NDIRECT + NINDIRECT)

typedef struct superblock {
  uint32_t size;        // Size of file system image (blocks)
  uint32_t nblocks;     // Number of data blocks
  uint32_t ninodes;     // Number of inodes.
  uint32_t inodestart;  // Block number of first inode block
  uint32_t bmapstart;   // Block number of first free map block
} superblock_t;

// On-disk inode structure
typedef struct dinode {
  short type;                   // File type
  short nlink;                  // Number of links to inode in file system
  uint32_t size;                // Size of file (bytes)
  uint32_t addrs[NDIRECT + 1];  // Data block addresses
} dinode_t;

typedef struct block {
  uint32_t blk_no;
  uint8_t data[BSIZE];
} block_t;

typedef struct dirent {
  uint16_t inum;
  char name[PATH_LENGTH];
} dirent_t;

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
void inode_print(inode_t* ip);

#endif
