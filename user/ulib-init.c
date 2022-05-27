#include <utils.h>
#include <ulib.h>

// #define USE_PRINTF

void printinfo(int pid, int i) {
  if (pid == 0) {
#ifndef USE_PRINTF
    char out[20] = "proc   i=xxx\n";
    out[9]       = '0' + (i / 100) % 10;
    out[10]      = '0' + (i / 10) % 10;
    out[11]      = '0' + i % 10;
    cputstr(out);
#else
    printf("@proc i=%d\n", i);
#endif
  } else {
#ifndef USE_PRINTF
    char out[20] = "fork pid=xxx\n";
    out[9]       = '0' + (pid / 100) % 10;
    out[10]      = '0' + (pid / 10) % 10;
    out[11]      = '0' + pid % 10;
    cputstr(out);
#else
    printf("@fork pid=%d\n", pid);
#endif
  }
}

int main(int argc, char const *argv[]) {
  for (int i = 0; i < 5; i++) {
    int pid = fork();
    printinfo(pid, i);
  }
  return 0;
}
