#include <fs.h>
#include <file.h>

filetable_t ftable;

// fd 0,1,2is used,so in processes' fdtable,value start from 3
// but in ftable it start from 0,so FILE(fd) need to minus 3
#define FILE(fd) (ftable.files + fd - 3)

void file_init() {
  spin_init(&ftable.lock, "ftable_lock");
}

/**
 * @brief alloc a available global fd
 *
 * @return int fd index in ftable
 */
int file_alloc() {
  file_t* f;
  // 0 1 2 used, so start from 3
  int fd = 3;
  spin_lock(&ftable.lock);
  for (f = ftable.files; f < ftable.files + FILE_TABLE_SIZE; f++) {
    // ref is 0 means no process hold this fd, can be reused.
    if (f->ref == 0) {
      f->ref  = 1;
      f->off  = 0;
      f->type = FD_INODE;
      f->iptr = NULL;
      spin_unlock(&ftable.lock);
      return fd;
    }
    fd++;
  }
  spin_unlock(&ftable.lock);
  panic("[file.c/file_alloc]:no file to alloc");
}

// dup just let file's and inode's ref + 1, and return the smallest available fd
int file_dup(file_t* f) {
  if (f == NULL) panic("[file->dup] dup a null file");
  spin_lock(&ftable.lock);

  if (f->ref < 1) panic("[file.c/file_dup] no ref");
  f->ref++;
  idup(f->iptr);
  int fd = 3;
  for (f = ftable.files; f < ftable.files + FILE_TABLE_SIZE; f++) {
    if (f->ref == 0) {
      spin_unlock(&ftable.lock);
      return fd;
    }
    fd++;
  }
  spin_unlock(&ftable.lock);
  return -1;
}

int file_stat(file_t* f, stat_t* st) {
  if (f == NULL) return -1;
  if (f->type == FD_INODE) {
    st->type      = f->iptr->type;
    st->dev       = f->iptr->dev->id;
    st->inode_num = f->iptr->inum;
    st->links     = f->iptr->nlink;
    st->size      = f->iptr->size;
    return 0;
  }
  return -1;
}

// close a file
void file_close(file_t* f) {
  if (f == NULL) panic("[file->close]f is null");
  spin_lock(&ftable.lock);

  if (f->ref < 1) {
    panic("[file->close]file already close!");
  }
  f->ref--;
  if (f->ref == 0) f->type = FD_NONE;
  spin_unlock(&ftable.lock);
  iput(f->iptr);
}

// read file's inode addrs' data
int file_read(file_t* f, char* buf, uint32_t n) {
  if (f == NULL || f->readable == 0) return -1;

  int len = 0;
  if (f->type == FD_INODE) {
    len = readi(f->iptr, buf, f->off, n);
    if (len > 0) {
      spin_lock(&ftable.lock);
      f->off += len;
      spin_unlock(&ftable.lock);
    }
    return len;
  }
  return -1;
}

int file_write(file_t* f, char* buf, uint32_t n) {
  if (f == NULL || f->writable == 0) return -1;

  int len = 0;

  if (f->type == FD_INODE) {
    len = writei(f->iptr, buf, f->off, n);
    if (len > 0) {
      spin_lock(&ftable.lock);
      f->off += len;
      spin_unlock(&ftable.lock);
    }
    return len;
  }
  return -1;
}

// get file by fd
file_t* file_get(uint32_t fd) {
  if (fd > FILE_TABLE_SIZE + 3 || fd < 3) return NULL;
  return FILE(fd);
}

void file_print_info(int level) {
  file_t* f;
  int used = 0;
  int fd   = -1;
  info("System FD table info:");
  for (int i = 0; i < FILE_TABLE_SIZE; i++) {
    f = ftable.files + i;
    if (f->type == FD_INODE) {
      used++;
      if (level) {
        printf("\tFILE %d:", i + 3);
        printf("\t ref:%d", f->ref);
        printf("\t off:%d\t", f->off);
        if (f->readable) printf("R");
        if (f->writable) printf("W");
        printf("\n");
      }
    } else if (fd == -1) {
      fd = i + 3;
    }
  }
  info("\tUsed system FD: %d", used);
  info("\tsmallest available system FD: %d", fd);
}

// MODULE_DEF(file) = {
//     .init  = file_init,
//     .alloc = file_alloc,
//     .dup   = file_dup,
//     .stat  = file_stat,
//     .close = file_close,
//     .read  = file_read,
//     .write = file_write,
//     .get   = file_get,
// };