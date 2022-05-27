#include <ulib.h>
#include <utils.h>

int main() {
  // test fork
  // int pid;
  // for (int i = 0; i < 2; i++) {
  //   kputstr("main for loop\n");
  //   if ((pid = fork()) != 0) {
  //     sleep(1);
  //   } else {
  //     kputstr("subproc!\n");
  //   }
  char s[100];
  sprintf(s, "Hello! %d\n", 123);
  cputstr(s);
  // int fd       = open("/dev/zero", O_RDONLY);
  // char buf[10] = {-1};
  // // will assert fault here: read not implemented
  // if (read(fd, buf, 1) != -1 && buf[0] == 0) {
  //   cputstr("success!\n");
  // } else {
  //   cputstr("error!\n");
  // }

  while (1) {
    print_time();
    cputstr(" hello from initcode!\n");
    // test open
    sleep(1);
  }
  while (1)
    sleep(1000);
  return 0;
}
