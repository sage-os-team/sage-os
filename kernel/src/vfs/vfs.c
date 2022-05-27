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

// create an inode bind to path
static inode_t *create(const char *path, short type) {
  struct inode *ip, *dp;
  char name[PATH_LENGTH];

  if (strcmp(path, DEV_ZERO_PATH) == 0) {
    return 0;
  } else if (strcmp(path, DEV_NULL_PATH) == 0) {
    return 0;
  } else if (strcmp(path, DEV_RANDOM_PATH) == 0) {
    return 0;
  } else if (strcmp(path, DEV_INPUT_PATH) == 0) {
    return 0;
  } else if (strcmp(path, DEV_VIDEO_PATH) == 0) {
    return 0;
  }

  if ((dp = nameiparent(path, name)) == 0) {
    warn("[vfs.c/create] nameiparent \"%s\".",path);
    return 0;
  }
  ilock(dp);

  if ((ip = dirlookup(dp, name, 0)) != 0) {
    iunlockput(dp);
    ilock(ip);
    if (type == DINODE_TYPE_F && ip->type == DINODE_TYPE_F) {
      warn("[vfs.c/create] path=\"%s\",file already exist.but the system still "
           "create it.",
           path);
      iunlockput(ip);
      return ip;
    }
    iunlockput(ip);
    warn("[vfs.c/create] path=\"%s\",dir already exist.", path);
    return 0;
  }

  ip = ialloc(type);
  assert_msg(ip != NULL, "[vfs.c/create] ialloc failed.");

  ilock(ip);
  ip->nlink = 1;
  iupdate(ip);

  if (type == DINODE_TYPE_D) {  // Create . and .. entries.
    dp->nlink++;                // for ".."
    iupdate(dp);
    // No ip->nlink++ for ".": avoid cyclic ref count.
    if (dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
      panic("[vfs.c/create]dots");
  }

  if (dirlink(dp, name, ip->inum) < 0) panic("[vfs.c/create] dirlink");

  iunlockput(dp);
  iunlock(ip);
  success("[vfs.c/create]create file/dirent %s successfully.", path);
  return ip;
}

static int isdirempty(inode_t *dp) {
  int off;
  dirent_t de;

  for (off = 2 * sizeof(de); off < dp->size; off += sizeof(de)) {
    if (readi(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
      panic("[vfs.c/isdirempty] readi");
    if (de.inum != 0) return 0;
  }
  return 1;
}

int _randint(int l, int r) {
  return l + (rand() & 0x7fffffff) % (r - l + 1);
}

void vfs_init() {
  // int i;
  // inodetable = pmm->alloc(sizeof(inode_t) * NBLOCK);
  // // read all inode into memory
  // for (i = 0; i < NBLOCK; i++) {
  //   fs->readinode(dev->lookup("sda"), i, inodetable + i);
  // }
  fs->init();

  // WARNING!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // operation behind need to move to mkfs.c
  // usage:create root dirent and /dev /uproc /usr

  dirent_t dir;
  memset(dir.name, 0, PATH_LENGTH);
  dir.inum = ROOTINO;
  strncpy(dir.name, ".", 1);
  writei(iget(ROOTINO), (char *)&dir, 0, sizeof(dirent_t));

  dir.inum = ROOTINO;
  strncpy(dir.name, "..", 2);
  writei(iget(ROOTINO), (char *)&dir, sizeof(dirent_t), sizeof(dirent_t));
  //create("/usr",DINODE_TYPE_D);
}

// open a file,return  the process fd(fdtable's subscript)
// rather than the system global fd
int sys_open(task_t *proc, const char *pathname, int flags) {
  inode_t *inode   = NULL;
  int available_fd = -1;  // process idel fd

  // find available process fd.if not found,return -1
  for (int i = 3; i < PROCESS_FILE_TABLE_SIZE; i++) {
    if (proc->fdtable[i] < 0) {
      available_fd = i;
      break;
    }
  }
  if (available_fd == -1) {
    warn("[sys_open] process fd table is full!");
    return -1;
  }
  // special device
  if (strcmp(pathname, DEV_ZERO_PATH) == 0) {
    proc->fdtable[available_fd] = DEV_ZERO_FD;
    return available_fd;
  } else if (strcmp(pathname, DEV_NULL_PATH) == 0) {
    proc->fdtable[available_fd] = DEV_NULL_FD;
    return available_fd;
  } else if (strcmp(pathname, DEV_RANDOM_PATH) == 0) {
    proc->fdtable[available_fd] = DEV_RANDOM_FD;
    return available_fd;
  } else if (strcmp(pathname, DEV_INPUT_PATH) == 0) {
    proc->fdtable[available_fd] = DEV_INPUT_FD;
    return available_fd;
  } else if (strcmp(pathname, DEV_VIDEO_PATH) == 0) {
    proc->fdtable[available_fd] = DEV_VIDEO_FD;
    return available_fd;
  }

  if (flags & O_CREAT) {
    inode = create((char *)pathname, DINODE_TYPE_F);
    assert_msg(inode != NULL, "[sys_open] failed to alloc inode");
    //获取一个新的文件描述符并绑定到inode
    int fd    = file_alloc();
    file_t *f = file_get(fd);
    if (f == NULL) {
      warn("[vfs.c/sys_open] no fd available.");
      iput(inode);
      return -1;
    }
    f->iptr     = inode;
    f->readable = !(flags & O_WRONLY);
    f->writable = (flags & O_WRONLY) || (flags & O_RDWR);
    //从fd_table找一个空的填入fd并返回fd
    for (int i = 3; i < PROCESS_FILE_TABLE_SIZE; i++) {
      if (proc->fdtable[i] < 0) {
        proc->fdtable[i] = fd;
        return i;
      }
    }
    panic("[sys_open] process fd table is full!");
    return -1;
  } else {  // flg=0表明没找到文件（inode此时为最大匹配）
    inode = namei((char *)pathname);
    assert_msg(inode != NULL, "[sys_open] failed to alloc inode");
    if (inode != NULL) {
      //获取一个新的文件描述符并绑定到inode
      int fd    = file_alloc();
      file_t *f = file_get(fd);
      if (f == NULL) {
        warn("[vfs.c/sys_open] no fd available.");
        iput(inode);
        return -1;
      }
      f->iptr     = inode;
      f->readable = !(flags & O_WRONLY);
      f->writable = (flags & O_WRONLY) || (flags & O_RDWR);
      for (int i = 3; i < PROCESS_FILE_TABLE_SIZE; i++) {
        if (proc->fdtable[i] < 0) {
          proc->fdtable[i] = fd;
          return i;
        }
      }
      panic("[sys_open] process fd table is full!");
    }
    return -1;
  }
}

int sys_close(task_t *proc, int fd) {
  int sys_fd        = proc->fdtable[fd];
  proc->fdtable[fd] = -1;
  if (sys_fd < 3 || sys_fd >= FILE_TABLE_SIZE) return 0;

  file_t *f = file_get(sys_fd);
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
      d = dev->lookup("tty1");
      return d->ops->read(d, 0, buf, nbyte);
    case 1:
    case 2:
      warn("[sys_read]WARNING!You are trying to read from stdout/stderr!");
      return -1;
    case DEV_ZERO_FD:
    case DEV_NULL_FD:
      return 0;
    case DEV_RANDOM_FD:
      return _randint(0, 256);
    case DEV_INPUT_FD:
      d = dev->lookup("input");
      return d->ops->read(d, 0, buf, nbyte);
    case DEV_VIDEO_FD:
      return 0;
    default:
      return file_read(file_get(proc->fdtable[fd]), buf, nbyte);
  }
}

int sys_write(task_t *proc, int fd, void *buf, size_t nbyte) {
  device_t *d;
  switch (proc->fdtable[fd]) {
    case 0:
      warn("[sys_write]WARNING!You are trying to write to stdin!");
      return -1;
    case 1:
    case 2:
      d = dev->lookup("tty1");
      return d->ops->write(d, 0, buf, nbyte);
    case DEV_ZERO_FD:
    case DEV_NULL_FD:
      return 0;
    case DEV_RANDOM_FD:
      return -1;
    case DEV_INPUT_FD:
      warn("[sys_write]WARNING!You are trying to write to input device!");
      return -1;
    case DEV_VIDEO_FD:
      return 0;
    default:
      return file_write(file_get(proc->fdtable[fd]), buf, nbyte);
  }
}

int sys_link(task_t *proc, const char *oldpath, const char *newpath) {
  char name[PATH_LENGTH];
  inode_t *ip, *dp;
  // find oldpath inode
  if ((ip = namei(oldpath)) == 0) return -1;
  ilock(ip);
  // inode is not file
  if (ip->type == DINODE_TYPE_D) {
    iunlockput(ip);
    return -1;
  }

  ip->nlink++;
  iupdate(ip);
  iunlock(ip);

  if ((dp = nameiparent(newpath, name)) == 0) {
    ilock(ip);
    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);
    return -1;
  }
  ilock(dp);
  if (dirlink(dp, name, ip->inum) < 0) {
    iunlockput(dp);
    ilock(ip);
    ip->nlink--;
    iupdate(ip);
    iunlockput(ip);
    return -1;
  }
  iunlockput(dp);
  iput(ip);
  return 0;
}

int sys_unlink(task_t *proc, const char *pathname) {
  char name[PATH_LENGTH];

  inode_t *ip, *dp;
  uint32_t off;
  dirent_t de;
  if ((dp = nameiparent(pathname, name)) == 0) {
    return -1;
  }
  ilock(dp);
  // can't unlink . and ..
  if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
    iunlockput(dp);
    return -1;
  }

  if ((ip = dirlookup(dp, name, &off)) == 0) {
    iunlockput(dp);
    return -1;
  }

  ilock(ip);
  if (ip->nlink < 1) {
    panic("[vfs.c/sys_unlink] nlink < 1.");
  }
  if (ip->type == DINODE_TYPE_D && !isdirempty(ip)) {
    iunlockput(ip);
    iunlockput(dp);
    return -1;
  }

  memset(&de, 0, sizeof(de));
  if (writei(dp, (char *)&de, off, sizeof(de)) != sizeof(de))
    panic("[vfs.c/sys_unlink] writei.");
  if (ip->type == DINODE_TYPE_D) {
    dp->nlink--;
    iupdate(dp);
  }
  iunlockput(dp);

  ip->nlink--;
  iupdate(ip);
  iunlockput(ip);

  return 0;
}

int sys_fstat(task_t *proc, int fd, stat_t *buf) {
  if (proc->fdtable[fd] <= 2) return -1;
  return file_stat(file_get(proc->fdtable[fd]), buf);
}

// create dir,return 1 if succeed,0 if fail.
int sys_mkdir(task_t *proc, const char *pathname) {
  inode_t *inode = create(pathname, DINODE_TYPE_D);
  if (inode != NULL) return 0;
  return -1;
}

int sys_chdir(task_t *proc, const char *pathname) {
  inode_t *ip;
  task_t *curproc = current_task;

  if ((ip = namei(pathname)) == 0) {
    return -1;
  }
  ilock(ip);
  if (ip->type != DINODE_TYPE_D) {
    iunlockput(ip);
    return -1;
  }
  iunlock(ip);
  iput(curproc->cwd);
  curproc->cwd = ip;
  return 0;
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

int sys_lseek(task_t *proc, int fd, int off) {
  file_t *f;
  switch (proc->fdtable[fd]) {
    case 0:
    case 1:
    case 2:
    case DEV_ZERO_FD:
    case DEV_NULL_FD:
    case DEV_RANDOM_FD:
    case DEV_INPUT_FD:
    case DEV_VIDEO_FD:
      warn("[sys_lseek]WARNING!You are trying to lseek for wrong place!");
      return -1;
    default:
      f = file_get(proc->fdtable[fd]);
      if (f != NULL) {
        f->off = off;
        return 0;
      } else {
        return -1;
      }
  }
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
    .lseek  = sys_lseek,
};