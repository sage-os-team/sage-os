#include "trap.h"

int main() {
  int friends = 11;
  char out[200];
  snprintf(out, 15, "Hello,I am %s,I have %05.3d friends.", "Tom", friends);
  check(strcmp(out, "Hello,I am Tom") == 0);
  sprintf(out, "Hello,I am %s,I have %5.3d friends.They are  at %#x.", "Tom",
          friends, friends);
  check(strcmp(out, "Hello,I am Tom,I have   010 friends.They are  at 0xa.") ==
        0);
  sprintf(out, "Hello,I am %s,I have %d friends.They are  at %#x.", "Tom",
          friends, friends);
  check(strcmp(out, "Hello,I am Tom,I have 10 friends.They are  at 0xa.") == 0);
  printf("Hello,I am %s,I have %ld friends.They are  at %#x or %p.", "Tom",
         friends, friends, &friends);
  return 0;
}
