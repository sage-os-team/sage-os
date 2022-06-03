#include <ulib.h>
#include <utils.h>

int main() {
  char s[100];
  sprintf(s, "Hello! %d\n", 123);
  cputstr(s);

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
