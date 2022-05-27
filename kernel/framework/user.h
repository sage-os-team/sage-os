#ifndef USER_H__
#define USER_H__

#define MAP_SHARED  1
#define MAP_PRIVATE 2
#define MAP_UNMAP   3

#define PROT_NONE  0x1
#define PROT_READ  0x2
#define PROT_WRITE 0x4

#include <stdint.h>

#define T_DIR  1
#define T_FILE 2

#define SEEK_CUR 0
#define SEEK_SET 1
#define SEEK_END 2

#define O_RDONLY 00000000
#define O_WRONLY 00000001
#define O_RDWR   00000002
#define O_CREAT  00000100

#define PATH_LENGTH 32

// see file.h
typedef struct {
  int type;
  int dev;
  uint32_t inode_num;
  int links;
  uint32_t size;
} _stat_t;

typedef struct {
  uint16_t inum;
  char name[PATH_LENGTH];
} __attribute__((packed)) _dirent_t;

#endif