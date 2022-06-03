#include <ulib.h>
#include <utils.h>

int main() {
  kputstr("hello123\n");
  cputstr("Please input your name:");
  char name[30];
  if (read(0, name, 30) > 0) {
    cputstr("\nWelcome, ");
    cputstr(name);
  } else
    cputstr("no input\n");
  while (1) {
    print_time();
    cputstr(" hello from initcode!\n");
    // test open
    sleep(1);
  }
  return 0;
}
