#include <kernel.h>
#include <klib.h>
#include <user.h>
#include <am.h>
#include <logger.h>
#include <file.h>
#include <fs.h>

void func(void* arg) {
  task_t* task = (task_t*)arg;

  //创建文件夹 以/开头是绝对路径
  vfs->mkdir(task, "usr");
  vfs->mkdir(task, "usr/1");
  vfs->mkdir(task, "/bin");
  vfs->mkdir(task, "/bin");
  vfs->mkdir(task, "bin/123");
  int fd = vfs->open(task, "/bin/123/1.txt", O_CREAT | O_RDWR);
  vfs->close(task, fd);
  //打开同一文件三次，前两次有标签O_CREAT,第二次打开后写入，第三次打开后读出
  int fd1 = vfs->open(task, "usr/1/te.c", O_CREAT | O_RDWR);
  file_print_info(1);
  //第二次打开，只写
  int fd2 = vfs->open(task, "/usr/1/te.c", O_CREAT | O_WRONLY);
  file_print_info(1);
  char buf[50];
  memset(buf, 0, sizeof(buf));
  //写入
  int w = vfs->write(task, fd1, "hello world!", 13);
  //第三次打开文件，只读
  int fd3 = vfs->open(task, "/usr/1/te.c", O_RDONLY);
  file_print_info(1);
  // lseek
  vfs->lseek(task, fd1, 0);
  int r = vfs->read(task, fd1, buf, 13);
  printf("%d %d %d %d %d\n", fd1, fd2, w, fd3, r);
  printf("data is:%s\n", buf);
  //关闭文件
  vfs->close(task, fd1);
  vfs->close(task, fd2);
  vfs->close(task, fd3);
  file_print_info(1);

  // for(int i=0;i<10;i++){
  //   inode_print(i);
  // }

  fd1 = vfs->open(task, "/usr/1/te.c", O_CREAT | O_RDWR);
  file_print_info(1);
  fd2 = vfs->open(task, "/usr/1/te.c", O_WRONLY);
  file_print_info(1);
  memset(buf, 0, sizeof(buf));
  w = vfs->write(task, fd1, "hello world version 2!", 23);
  w = vfs->write(task, fd1, "hello world version 3!", 23);
  w = vfs->write(task, fd1, "hello world version 4!", 23);
  //尝试从只写文件读出
  r = vfs->read(task, fd2, buf, 23);
  printf("%d %d %d %d\n", fd1, fd2, w, r);
  printf("fd 2 data is:%s\n", buf);
  // file dup
  fd3 = vfs->dup(task, fd1);
  vfs->lseek(task, fd1, 23);
  r = vfs->read(task, fd3, buf, 23);
  printf("fd 3 data is:%s\n", buf);
  vfs->close(task, fd1);
  vfs->close(task, fd2);
  vfs->close(task, fd3);
  file_print_info(1);

  fd1 = vfs->open(task, "/dev/random", O_CREAT);
  r   = vfs->read(task, fd1, 0, 0);
  printf("read from RANDOM is %d\n", r);

  r = vfs->mkdir(task, "/dev/input");
  printf("mkdri result is %d\n", r);

  //两文件同名
  r = vfs->link(task, "/usr/1/te.c", "/a.txt");

  fd1 = vfs->open(task, "/a.txt", O_RDWR);
  memset(buf, 0, sizeof(buf));
  r = vfs->read(task, fd1, buf, 23);
  printf("a.txt data is:%s\n", buf);

  //将当前目录移动到 /usr/1
  vfs->chdir(task, "/usr/1");
  fd1 = vfs->open(task, "te.c", O_RDWR);
  memset(buf, 0, sizeof(buf));
  vfs->lseek(task, fd1, 46);
  r = vfs->read(task, fd1, buf, 23);
  printf("change:data is:%s\n", buf);

  //解除链接，相当于删除当前文件（如果该文件没有其他链接或引用）
  r   = vfs->unlink(task, "/a.txt");
  r   = vfs->unlink(task, "te.c");
  fd1 = vfs->open(task, "te.c", O_CREAT | O_RDWR);

  r = vfs->unlink(task, "/bin");
  printf("close result:%d.\n", r);

  while (1)
    ;
  // vfs->link(task,"","/uproc/task1");
}

int main() {
  _log_mask = LOG_ERROR | LOG_INFO | LOG_WARN | LOG_SUCCESS;
  ioe_init();
  cte_init(os->trap);
  os->init();
  vme_init(pmm->pgalloc, pmm->free);
  uproc->init();
  vfs->init();

  task_t* task = pmm->alloc(sizeof(task_t));
  kmt->create(task, "test", func, task);

  mpe_init(os->run);
  return 1;
}