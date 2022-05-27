#include <ulib.h>
#include <utils.h>

int main() {
  int fd = open("/abc.txt", O_CREAT | O_RDWR);
  write(fd, "hello world", 11);
  char buf[20];
  read(fd, buf, sizeof buf);
  cputstr(buf);
  close(fd);
  cputstr("finish!");
  while (1)
    ;
  return 0;
}
