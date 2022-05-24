#ifndef __FILE_H__
#define __FILE_H__

#include <common.h>
#include <sem.h>
#include <spinlock.h>
#include <devices.h>
#include <fs.h>

#define FILE_TABLE_SIZE         100
#define PROCESS_FILE_TABLE_SIZE 16

typedef struct file {
  enum { FD_NONE, FD_INODE } type;
  int ref;  // reference count
  char readable;
  char writable;
  struct inode* iptr;
  uint32_t off;
} file_t;

typedef struct file_table {
  spinlock_t lock;
  file_t files[FILE_TABLE_SIZE];
} filetable_t;

typedef struct file_stat {
  enum { ST_DIR, ST_FILE, ST_DEV } type;  // file type:directory,file,device
  int dev;                                // file on which device
  uint32_t inode_num;
  int links;      // number of links to file
  uint32_t size;  // file size
} stat_t;

// in-memory copy of an inode
typedef struct inode {
  device_t* dev;    // Device
  uint32_t inum;    // Inode number
  int ref;          // Reference count
  spinlock_t lock;  // protects everything below here
  // delete valid because all inodes will be in memory at boot
  // int valid;

  short type;                   // File type
  short nlink;                  // Number of links to inode in file system
  uint32_t size;                // Size of file (bytes)
  uint32_t addrs[NDIRECT + 1];  // Data block addresses
} inode_t;

void file_init();
int file_alloc();
int file_dup(file_t* f);
int file_stat(file_t* f, stat_t* st);
void file_close(file_t* f);
int file_read(file_t* f, char* buf, uint32_t n);
int file_write(file_t* f, char* buf, uint32_t n);
file_t* file_get(uint32_t fd);

#endif