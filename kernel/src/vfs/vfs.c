#include <vfs.h>
#include <kernel.h>
#include <syscall_defs.h>
#include <user.h>
#include <syscalls.h>
#include <thread.h>
#include <fs.h>
#include <file.h>
#include <devices.h>

// inode_t *inodetable;
// #define rootinode inodetable

void vfs_init() {
  // int i;
  // inodetable = pmm->alloc(sizeof(inode_t) * NBLOCK);
  // // read all inode into memory
  // for (i = 0; i < NBLOCK; i++) {
  //   fs->readinode(dev->lookup("sda"), i, inodetable + i);
  // }
  fs->init();
}

// int sys_open(task_t *proc, const char *pathname, int flags) {
//   inode_t *inode = NULL;
//
//   if (flags == O_CREAT) {
//     // inode=create(pathname,1);
//     if (inode == NULL) return -1;
//     //获取一个新的文件描述符并绑定到inode
//     int fd = file->alloc();
//     if (file->get(fd) != NULL) file->get(fd)->iptr = inode;
//     //从fd_table找一个空的填入fd并返回fd
//     for (int i = 3; i < PROCESS_FILE_TABLE_SIZE; i++) {
//       // if(proc->fdtable[i]==0){
//       //   proc->fdtable[i]=fd;
//       //   return i;
//       // }
//     }
//     return -1;
//   } else {  // flg=0表明没找到文件（inode此时为最大匹配）
//     int flg = 0;
//     // flg=find_inode_by_pathname(pathname,inode);
//     if (inode != NULL && flg == 1) {
//       //获取一个新的文件描述符并绑定到inode
//       int fd = file->alloc();
//       if (file->get(fd) != NULL) file->get(fd)->iptr = inode;
//       if (flags == O_RDWR) {
//         file->get(fd)->readable = 1;
//         file->get(fd)->writable = 1;
//       } else if (flags == O_RDONLY) {
//         file->get(fd)->readable = 1;
//         file->get(fd)->writable = 0;
//       } else {
//         file->get(fd)->readable = 0;
//         file->get(fd)->writable = 1;
//       }
//       for (int i = 3; i < PROCESS_FILE_TABLE_SIZE; i++) {
//         // if(proc->fdtable[i]==0){
//         //   proc->fdtable[i]=fd;
//         //   return i;
//         // }
//       }
//     }
//     return -1;
//   }
// }
//
// int sys_close(task_t *proc, int fd) {
//   //   file_t *f = file->get(proc->fdtable[fd]);
//   //   proc->fdtable[fd]=-1;
//   //   if (f != NULL) {
//   //     file->close(f);
//   //   } else {
//   //     return -1;
//   //   }
//   // }
//   return 0;
// }
//
// int sys_read(task_t *proc, int fd, void *buf, size_t nbyte) {
//   device_t *d;
//   switch (proc->fdtable[fd]) {
//     case 0:
//     case 1:
//     case 2:
//       d = dev->lookup("tty1");
//       return d->ops->read(d, 0, buf, nbyte);
//     default:
//       return file->read(file->get(proc->fdtable[fd]), buf, nbyte);
//   }
// }
//
// int sys_write(task_t *proc, int fd, void *buf, size_t nbyte) {
//   device_t *d;
//   switch (proc->fdtable[fd]) {
//     case 0:
//     case 1:
//     case 2:
//       d = dev->lookup("tty1");
//       return d->ops->write(d, 0, buf, nbyte);
//     default:
//       return file->write(file->get(proc->fdtable[fd]), buf, nbyte);
//   }
// }
//
// int sys_link(task_t *proc, const char *oldpath, const char *newpath) {
//   assert_msg(false, "sys_link not implemented");
//   return 1;
// }
//
// int sys_unlink(task_t *proc, const char *pathname) {
//   assert_msg(false, "sys_unlink not implemented");
//   return 1;
// }
//
// int sys_fstat(task_t *proc, int fd, stat_t *buf) {
//   if (proc->fdtable[fd] <= 2) return -1;
//   return file->stat(file->get(proc->fdtable[fd]), buf);
// }
//
// int sys_mkdir(task_t *proc, const char *pathname) {
//   assert_msg(false, "sys_mkdir not implemented");
//   return 1;
// }
//
// int sys_chdir(task_t *proc, const char *path) {
//   assert_msg(false, "sys_chdir not implemented");
//   return 1;
// }
//
// int sys_dup(task_t *proc, int fd) {
//   for (int i = 0; i < PROCESS_FILE_TABLE_SIZE; i++) {
//     if (proc->fdtable[i] == -1) {
//       proc->fdtable[i] = proc->fdtable[fd];
//       return file->dup(file->get(proc->fdtable[fd]));
//     }
//   }
//   return -1;
// }

// open a file,return  the process fd(fdtable's subscript)
// rather than the system global fd
int sys_open(task_t *proc, const char *pathname, int flags) {
  inode_t *inode = NULL;

  if (flags & O_CREAT) {
    // inode=create(pathname,1);
    if (inode == NULL) return -1;
    //获取一个新的文件描述符并绑定到inode
    int fd    = file_alloc();
    file_t *f = file_get(fd);
    assert_msg(f != NULL, "[sys_open] failed to alloc file");
    f->iptr = inode;
    if (flags & O_RDWR) {
      f->readable = 1;
      f->writable = 1;
    } else if (flags & O_RDONLY) {
      f->readable = 1;
      f->writable = 0;
    } else {
      f->readable = 0;
      f->writable = 1;
    }
    // 从fd_table找一个空的填入fd并返回fd
    for (int i = 3; i < PROCESS_FILE_TABLE_SIZE; i++) {
      if (proc->fdtable[i] < 0) {
        proc->fdtable[i] = fd;
        return i;
      }
    }
    panic("[sys_open] process fd table is full!");
    return -1;
  } else {
    // flg=0表明没找到文件（inode此时为最大匹配）
    int flg = 0;
    // flg=find_inode_by_pathname(pathname,inode);
    if (inode != NULL && flg == 1) {
      //获取一个新的文件描述符并绑定到inode
      int fd    = file_alloc();
      file_t *f = file_get(fd);
      if (f != NULL) f->iptr = inode;
      if (flags & O_RDWR) {
        f->readable = 1;
        f->writable = 1;
      } else if (flags & O_RDONLY) {
        f->readable = 1;
        f->writable = 0;
      } else {
        f->readable = 0;
        f->writable = 1;
      }
      for (int i = 3; i < PROCESS_FILE_TABLE_SIZE; i++) {
        if (proc->fdtable[i] < 0) {
          proc->fdtable[i] = fd;
          return i;
        }
      }
    }
    return -1;
  }
}

int sys_close(task_t *proc, int fd) {
  file_t *f         = file_get(proc->fdtable[fd]);
  proc->fdtable[fd] = -1;
  if (f != NULL) {
    file_close(f);
  } else {
    return -1;
  }

  return 0;
}

int sys_read(task_t *proc, int fd, void *buf, size_t nbyte) {
  device_t *d;
  switch (proc->fdtable[fd]) {
    case 0:
    case 1:
    case 2:
      d = dev->lookup("tty1");
      return d->ops->read(d, 0, buf, nbyte);
    default:
      return file_read(file_get(proc->fdtable[fd]), buf, nbyte);
  }
}

int sys_write(task_t *proc, int fd, void *buf, size_t nbyte) {
  device_t *d;
  switch (proc->fdtable[fd]) {
    case 0:
    case 1:
    case 2:
      d = dev->lookup("tty1");
      return d->ops->write(d, 0, buf, nbyte);
    default:
      return file_write(file_get(proc->fdtable[fd]), buf, nbyte);
  }
}

int sys_link(task_t *proc, const char *oldpath, const char *newpath) {
  assert_msg(false, "sys_link not implemented");
  return 1;
}

int sys_unlink(task_t *proc, const char *pathname) {
  assert_msg(false, "sys_unlink not implemented");
  return 1;
}

int sys_fstat(task_t *proc, int fd, stat_t *buf) {
  if (proc->fdtable[fd] <= 2) return -1;
  return file_stat(file_get(proc->fdtable[fd]), buf);
}

int sys_mkdir(task_t *proc, const char *pathname) {
  assert_msg(false, "sys_mkdir not implemented");
  return 1;
}

int sys_chdir(task_t *proc, const char *path) {
  assert_msg(false, "sys_chdir not implemented");
  return 1;
}

// dup a file,return the process fd(fdtable's subscript)
// rather than the system global fd
int sys_dup(task_t *proc, int fd) {
  for (int i = 0; i < PROCESS_FILE_TABLE_SIZE; i++) {
    if (proc->fdtable[i] == -1) {
      proc->fdtable[i] = proc->fdtable[fd];
      if (proc->fdtable[fd] <= 2)  // stdin,out,err
        return i;
      else
        return (file_dup(file_get(proc->fdtable[fd])) == -1) ? -1 : i;
    }
  }
  return -1;
}

MODULE_DEF(vfs) = {
    .init   = vfs_init,
    .write  = sys_write,
    .read   = sys_read,
    .close  = sys_close,
    .open   = sys_open,
    .link   = sys_link,
    .unlink = sys_unlink,
    .fstat  = sys_fstat,
    .mkdir  = sys_mkdir,
    .chdir  = sys_chdir,
    .dup    = sys_dup,
};