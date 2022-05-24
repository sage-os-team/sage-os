#include <utils.h>

size_t strlen(char* s) {
  char* ptr = s;
  while (*ptr)
    ptr++;
  return ptr - s;
}

int cputstr(char* s) {
  return write(0, s, strlen(s) + 1);
}

void print_time() {
  int64_t t   = uptime();  // ms
  char st[10] = {(t / 10000) % 10 + '0', (t / 1000) % 10 + '0',
                 (t / 100) % 10 + '0',   (t / 10) % 10 + '0',
                 t % 10 + '0',           '\0'};
  cputstr(st);
}

char getc() {
  char ch;
  if (read(1, &ch, 1) != -1) {
    return ch;
  } else {
    return '\0';
  }
}
