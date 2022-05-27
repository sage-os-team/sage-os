#include <ulib.h>
#include <utils.h>

int main() {
  cputstr("Please input file name:");
  char path[30];
  if (read(0, path, 30) < 0) {
    kputstr("no input\n");
    while (1)
      ;
  }
  int fd = open(path, O_CREAT | O_RDWR);
  if (fd < 0) {
    cputstr("error");
  } else {
    char buf[200];
    for (int i = 0; i < 100; i++)
      buf[i] = 'a' + i % 26;
    buf[100] = '\0';
    if (write(fd, buf, sizeof buf) < 0) {
      cputstr("error");
    } else {
      for (int i = 0; i < 100; i++)
        buf[i] = 0;
      fd = dup(fd);
      if (read(fd, buf, sizeof(buf)) < 0) {
        cputstr("error");
      } else {
        cputstr(buf);
        kputstr(buf);
      }
    }
  }
  close(fd);
  cputstr("finish!");
  while (1)
    ;
  return 0;
}
