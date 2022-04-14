#include "trap.h"

void init(char *a,char *b,char *c)
{
    strcpy(a,"Welcome to sage-os!");
    strcpy(b,"Welcome");
    strcpy(c,"000000000000000000");
}
int main()
{
  char a[30]="Welcome to sage-os!";
  char b[30]="Welcome";
  char c[30];
  char d[20]="OOOOOO";
  char e[20]=" to sage-os!";
  check((strlen(b)==7)==1);
  check((strcmp(strncpy(c,a,7),b)==0)==1);
  init(a,b,c);
  check((strcmp(strcpy(c,a),a)==0)==1);
  init(a,b,c);
  check((strcmp(strcat(b,e),a)==0)==1);
  init(a,b,c);
  check((strncmp(a,b,7)==0)==1);
  init(a,b,c);
  check((memcmp(memset(c,'O',7),d,6)==0)==1);
  init(a,b,c);
  check((memcmp(memcpy(c,a,7),b,7)==0)==1);
  init(a,b,c);
  check((memcmp(memmove(c,a,7),b,7)==0)==1);
  init(a,b,c);
  check((memcmp(a,b,7)==0)==1);
  return 0;
}
